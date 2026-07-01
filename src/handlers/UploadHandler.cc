#include "UploadHandler.h"

#include <drogon/HttpTypes.h>
#include <drogon/MultiPart.h>
#include <json/json.h>

#include <filesystem>

#include "../ProcessRunner.h"
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

}  // namespace

namespace UploadHandler {

void handle(const HttpRequestPtr &req,
            std::function<void(const HttpResponsePtr &)> &&callback) {
    MultiPartParser parser;
    if (parser.parse(req) != 0) {
        callback(jsonError(k400BadRequest, "invalid multipart body"));
        return;
    }

    const auto &files = parser.getFiles();
    if (files.empty()) {
        callback(jsonError(k400BadRequest, "no file uploaded"));
        return;
    }

    const auto &file = files[0];
    std::string sessionId = SessionManager::newSessionId();
    auto sessionDir = SessionManager::sessionDirFor(sessionId);
    auto framesDir = SessionManager::framesDirFor(sessionId);

    std::error_code ec;
    std::filesystem::create_directories(framesDir, ec);
    if (ec) {
        callback(jsonError(k500InternalServerError, "failed to create session directory"));
        return;
    }

    std::filesystem::path originalName = file.getFileName();
    std::string ext = originalName.has_extension() ? originalName.extension().string() : ".mp4";
    auto videoPath = sessionDir / ("source" + ext);

    if (file.saveAs(videoPath.string()) != 0) {
        callback(jsonError(k500InternalServerError, "failed to save upload"));
        return;
    }

    double durationSec = ProcessRunner::getDurationSeconds(videoPath);
    if (durationSec < 0) {
        std::error_code rmEc;
        std::filesystem::remove_all(sessionDir, rmEc);
        callback(jsonError(k400BadRequest, "invalid or corrupt video file"));
        return;
    }

    Json::Value body;
    body["session_id"] = sessionId;
    body["duration_sec"] = durationSec;
    callback(HttpResponse::newHttpJsonResponse(body));
}

}  // namespace UploadHandler
