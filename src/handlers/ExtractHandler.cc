#include "ExtractHandler.h"

#include <drogon/HttpAppFramework.h>
#include <drogon/HttpTypes.h>
#include <json/json.h>

#include <algorithm>
#include <atomic>
#include <filesystem>
#include <thread>
#include <vector>

#include "../ProcessRunner.h"
#include "../SessionManager.h"

using namespace drogon;

namespace {

std::atomic<bool> g_extractionInProgress{false};

HttpResponsePtr jsonError(HttpStatusCode code, const std::string &message) {
    Json::Value body;
    body["error"] = message;
    auto resp = HttpResponse::newHttpJsonResponse(body);
    resp->setStatusCode(code);
    return resp;
}

}  // namespace

namespace ExtractHandler {

void handle(const HttpRequestPtr &req,
            std::function<void(const HttpResponsePtr &)> &&callback) {
    auto json = req->getJsonObject();
    if (!json) {
        callback(jsonError(k400BadRequest, "invalid json"));
        return;
    }

    std::string sessionId = (*json)["session_id"].asString();
    double interval = (*json)["interval"].asDouble();

    if (!SessionManager::isValidSessionId(sessionId)) {
        callback(jsonError(k400BadRequest, "invalid session_id"));
        return;
    }
    if (interval < 0.1) {
        callback(jsonError(k400BadRequest, "interval must be a positive number (>= 0.1)"));
        return;
    }

    auto videoPath = SessionManager::videoPathFor(sessionId);
    if (videoPath.empty() || !std::filesystem::exists(videoPath)) {
        callback(jsonError(k404NotFound, "session not found"));
        return;
    }
    auto framesDir = SessionManager::framesDirFor(sessionId);

    bool expected = false;
    if (!g_extractionInProgress.compare_exchange_strong(expected, true)) {
        callback(jsonError(k409Conflict, "extraction already in progress"));
        return;
    }

    auto *loop = drogon::app().getLoop();

    std::thread([loop, videoPath, framesDir, sessionId, interval,
                 callback = std::move(callback)]() mutable {
        bool ok = ProcessRunner::extractFrames(videoPath, framesDir, interval);
        g_extractionInProgress.store(false);

        HttpResponsePtr resp;
        if (!ok) {
            resp = jsonError(k500InternalServerError, "ffmpeg extraction failed");
        } else {
            std::vector<std::string> names;
            std::error_code ec;
            for (const auto &entry : std::filesystem::directory_iterator(framesDir, ec)) {
                if (entry.is_regular_file() && entry.path().extension() == ".jpg") {
                    names.push_back(entry.path().filename().string());
                }
            }
            std::sort(names.begin(), names.end());

            Json::Value frames(Json::arrayValue);
            for (const auto &name : names) {
                frames.append("/data/" + sessionId + "/frames/" + name);
            }
            Json::Value body;
            body["frames"] = frames;
            resp = HttpResponse::newHttpJsonResponse(body);
        }

        loop->queueInLoop([callback, resp]() { callback(resp); });
    }).detach();
}

}  // namespace ExtractHandler
