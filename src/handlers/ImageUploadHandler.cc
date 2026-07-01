#include "ImageUploadHandler.h"

#include <drogon/HttpTypes.h>
#include <drogon/MultiPart.h>
#include <json/json.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>
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

std::string lowerExt(const std::filesystem::path &name) {
    std::string ext = name.has_extension() ? name.extension().string() : "";
    std::transform(ext.begin(), ext.end(), ext.begin(),
                    [](unsigned char c) { return std::tolower(c); });
    return ext;
}

bool isAllowedImageExt(const std::string &ext) {
    return ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".webp" || ext == ".bmp";
}

}  // namespace

namespace ImageUploadHandler {

void handle(const HttpRequestPtr &req,
            std::function<void(const HttpResponsePtr &)> &&callback) {
    MultiPartParser parser;
    if (parser.parse(req) != 0) {
        callback(jsonError(k400BadRequest, "invalid multipart body"));
        return;
    }

    const auto &files = parser.getFiles();
    if (files.empty()) {
        callback(jsonError(k400BadRequest, "no files uploaded"));
        return;
    }

    std::string sessionId = SessionManager::newSessionId();
    auto framesDir = SessionManager::framesDirFor(sessionId);

    std::error_code ec;
    std::filesystem::create_directories(framesDir, ec);
    if (ec) {
        callback(jsonError(k500InternalServerError, "failed to create session directory"));
        return;
    }

    std::vector<std::string> savedNames;
    int idx = 1;
    for (const auto &file : files) {
        std::string ext = lowerExt(file.getFileName());
        if (!isAllowedImageExt(ext)) continue;

        char stem[16];
        std::snprintf(stem, sizeof(stem), "img_%04d", idx);
        auto dest = framesDir / (std::string(stem) + ext);
        if (file.saveAs(dest.string()) != 0) continue;

        savedNames.push_back(dest.filename().string());
        ++idx;
    }

    if (savedNames.empty()) {
        std::error_code rmEc;
        std::filesystem::remove_all(SessionManager::sessionDirFor(sessionId), rmEc);
        callback(jsonError(k400BadRequest, "no valid image files uploaded"));
        return;
    }

    std::sort(savedNames.begin(), savedNames.end());
    Json::Value frames(Json::arrayValue);
    for (const auto &name : savedNames) {
        frames.append("/data/" + sessionId + "/frames/" + name);
    }

    Json::Value body;
    body["session_id"] = sessionId;
    body["frames"] = frames;
    callback(HttpResponse::newHttpJsonResponse(body));
}

}  // namespace ImageUploadHandler
