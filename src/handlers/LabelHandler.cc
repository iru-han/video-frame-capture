#include "LabelHandler.h"

#include <drogon/HttpTypes.h>
#include <json/json.h>

#include <algorithm>
#include <fstream>
#include <sstream>
#include <vector>

#include "../SessionManager.h"

using namespace drogon;

namespace {

HttpResponsePtr jsonError(HttpStatusCode code, const std::string &message) {
    Json::Value body;
    body["error"] = message;
    auto resp = HttpResponse::newHttpJsonResponse(body);
    resp->setStatusCode(code);
    return resp;
}

// Rejects filenames containing path separators or "..", matching the safety
// check used by DeleteFrameHandler.
bool looksLikeSafeFilename(const std::string &filename) {
    if (filename.empty()) return false;
    if (filename.find('/') != std::string::npos) return false;
    if (filename.find('\\') != std::string::npos) return false;
    if (filename.find("..") != std::string::npos) return false;
    return true;
}

std::string trim(const std::string &s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

bool isValidClassName(const std::string &name) {
    if (name.empty() || name.size() > 64) return false;
    return name.find('\n') == std::string::npos && name.find('\r') == std::string::npos;
}

std::vector<std::string> readClasses(const std::filesystem::path &path) {
    std::vector<std::string> classes;
    std::ifstream in(path);
    std::string line;
    while (std::getline(in, line)) {
        std::string t = trim(line);
        if (!t.empty()) classes.push_back(t);
    }
    return classes;
}

std::filesystem::path labelPathForFrame(const std::string &sessionId,
                                         const std::string &filename) {
    std::filesystem::path stem = std::filesystem::path(filename).stem();
    return SessionManager::labelsDirFor(sessionId) / (stem.string() + ".txt");
}

}  // namespace

namespace LabelHandler {

void getClasses(const HttpRequestPtr &req,
                 std::function<void(const HttpResponsePtr &)> &&callback) {
    std::string sessionId = req->getParameter("session_id");
    if (!SessionManager::isValidSessionId(sessionId)) {
        callback(jsonError(k400BadRequest, "invalid session_id"));
        return;
    }

    auto classes = readClasses(SessionManager::classesFileFor(sessionId));
    Json::Value arr(Json::arrayValue);
    for (const auto &c : classes) arr.append(c);

    Json::Value body;
    body["classes"] = arr;
    callback(HttpResponse::newHttpJsonResponse(body));
}

void postClass(const HttpRequestPtr &req,
               std::function<void(const HttpResponsePtr &)> &&callback) {
    auto json = req->getJsonObject();
    if (!json) {
        callback(jsonError(k400BadRequest, "invalid json"));
        return;
    }

    std::string sessionId = (*json)["session_id"].asString();
    std::string className = trim((*json)["class"].asString());

    if (!SessionManager::isValidSessionId(sessionId)) {
        callback(jsonError(k400BadRequest, "invalid session_id"));
        return;
    }
    if (!isValidClassName(className)) {
        callback(jsonError(k400BadRequest, "invalid class name"));
        return;
    }

    auto sessionDir = SessionManager::sessionDirFor(sessionId);
    std::error_code ec;
    if (!std::filesystem::exists(sessionDir, ec)) {
        callback(jsonError(k404NotFound, "session not found"));
        return;
    }

    auto classesFile = SessionManager::classesFileFor(sessionId);
    auto classes = readClasses(classesFile);
    if (std::find(classes.begin(), classes.end(), className) == classes.end()) {
        std::ofstream out(classesFile, std::ios::app);
        out << className << "\n";
        classes.push_back(className);
    }

    Json::Value arr(Json::arrayValue);
    for (const auto &c : classes) arr.append(c);
    Json::Value body;
    body["classes"] = arr;
    callback(HttpResponse::newHttpJsonResponse(body));
}

void getLabel(const HttpRequestPtr &req,
              std::function<void(const HttpResponsePtr &)> &&callback) {
    std::string sessionId = req->getParameter("session_id");
    std::string filename = req->getParameter("filename");

    if (!SessionManager::isValidSessionId(sessionId) || !looksLikeSafeFilename(filename)) {
        callback(jsonError(k400BadRequest, "invalid session_id or filename"));
        return;
    }

    auto classes = readClasses(SessionManager::classesFileFor(sessionId));
    auto labelPath = labelPathForFrame(sessionId, filename);

    Json::Value boxes(Json::arrayValue);
    std::ifstream in(labelPath);
    std::string line;
    while (std::getline(in, line)) {
        std::istringstream iss(line);
        int classId;
        double xc, yc, w, h;
        if (!(iss >> classId >> xc >> yc >> w >> h)) continue;
        if (classId < 0 || classId >= static_cast<int>(classes.size())) continue;

        Json::Value box;
        box["class"] = classes[classId];
        box["x_center"] = xc;
        box["y_center"] = yc;
        box["width"] = w;
        box["height"] = h;
        boxes.append(box);
    }

    Json::Value body;
    body["boxes"] = boxes;
    callback(HttpResponse::newHttpJsonResponse(body));
}

void getLabeledFrames(const HttpRequestPtr &req,
                       std::function<void(const HttpResponsePtr &)> &&callback) {
    std::string sessionId = req->getParameter("session_id");
    if (!SessionManager::isValidSessionId(sessionId)) {
        callback(jsonError(k400BadRequest, "invalid session_id"));
        return;
    }

    auto labelsDir = SessionManager::labelsDirFor(sessionId);
    Json::Value labeled(Json::arrayValue);
    std::error_code ec;
    if (std::filesystem::exists(labelsDir, ec)) {
        for (const auto &entry : std::filesystem::directory_iterator(labelsDir, ec)) {
            if (entry.is_regular_file() && entry.path().extension() == ".txt") {
                labeled.append(entry.path().stem().string());
            }
        }
    }

    Json::Value body;
    body["labeled"] = labeled;
    callback(HttpResponse::newHttpJsonResponse(body));
}

void postLabel(const HttpRequestPtr &req,
               std::function<void(const HttpResponsePtr &)> &&callback) {
    auto json = req->getJsonObject();
    if (!json) {
        callback(jsonError(k400BadRequest, "invalid json"));
        return;
    }

    std::string sessionId = (*json)["session_id"].asString();
    std::string filename = (*json)["filename"].asString();

    if (!SessionManager::isValidSessionId(sessionId) || !looksLikeSafeFilename(filename)) {
        callback(jsonError(k400BadRequest, "invalid session_id or filename"));
        return;
    }

    auto classes = readClasses(SessionManager::classesFileFor(sessionId));
    if (classes.empty()) {
        callback(jsonError(k400BadRequest, "no classes registered for this session"));
        return;
    }

    const auto &boxesJson = (*json)["boxes"];
    if (!boxesJson.isArray()) {
        callback(jsonError(k400BadRequest, "boxes must be an array"));
        return;
    }

    std::ostringstream out;
    for (const auto &box : boxesJson) {
        std::string className = box["class"].asString();
        auto it = std::find(classes.begin(), classes.end(), className);
        if (it == classes.end()) {
            callback(jsonError(k400BadRequest, "unknown class: " + className));
            return;
        }
        int classId = static_cast<int>(std::distance(classes.begin(), it));

        double xc = box["x_center"].asDouble();
        double yc = box["y_center"].asDouble();
        double w = box["width"].asDouble();
        double h = box["height"].asDouble();
        if (xc < 0 || xc > 1 || yc < 0 || yc > 1 || w <= 0 || w > 1 || h <= 0 || h > 1) {
            callback(jsonError(k400BadRequest, "box coordinates must be normalized to [0,1]"));
            return;
        }

        out << classId << " " << xc << " " << yc << " " << w << " " << h << "\n";
    }

    auto labelsDir = SessionManager::labelsDirFor(sessionId);
    std::error_code ec;
    std::filesystem::create_directories(labelsDir, ec);
    if (ec) {
        callback(jsonError(k500InternalServerError, "failed to create labels directory"));
        return;
    }

    auto labelPath = labelPathForFrame(sessionId, filename);
    std::ofstream outFile(labelPath, std::ios::trunc);
    if (!outFile) {
        callback(jsonError(k500InternalServerError, "failed to write label file"));
        return;
    }
    outFile << out.str();

    Json::Value body;
    body["ok"] = true;
    callback(HttpResponse::newHttpJsonResponse(body));
}

}  // namespace LabelHandler
