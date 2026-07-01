#include "DeleteFrameHandler.h"

#include <drogon/HttpTypes.h>
#include <json/json.h>

#include <filesystem>

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

// Rejects filenames containing path separators or "..", which combined with
// the canonical-path containment check below makes path traversal (e.g.
// "../../etc/passwd") impossible.
bool looksLikeSafeFilename(const std::string &filename) {
    if (filename.empty()) return false;
    if (filename.find('/') != std::string::npos) return false;
    if (filename.find('\\') != std::string::npos) return false;
    if (filename.find("..") != std::string::npos) return false;
    return true;
}

}  // namespace

namespace DeleteFrameHandler {

void handle(const HttpRequestPtr &req,
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

    auto framesDir = SessionManager::framesDirFor(sessionId);
    std::error_code ec;
    auto canonicalFramesDir = std::filesystem::weakly_canonical(framesDir, ec);
    if (ec) {
        callback(jsonError(k404NotFound, "session not found"));
        return;
    }

    auto candidate = framesDir / filename;
    auto canonicalCandidate = std::filesystem::weakly_canonical(candidate, ec);
    if (ec) {
        callback(jsonError(k400BadRequest, "invalid filename"));
        return;
    }

    // Defense in depth: re-verify the resolved path is actually contained
    // within the session's frames directory, not just string-checked above.
    auto mismatch = std::mismatch(canonicalFramesDir.begin(), canonicalFramesDir.end(),
                                   canonicalCandidate.begin());
    if (mismatch.first != canonicalFramesDir.end()) {
        callback(jsonError(k400BadRequest, "invalid filename"));
        return;
    }

    if (!std::filesystem::exists(canonicalCandidate)) {
        callback(jsonError(k404NotFound, "file not found"));
        return;
    }

    std::filesystem::remove(canonicalCandidate, ec);
    if (ec) {
        callback(jsonError(k500InternalServerError, "failed to delete"));
        return;
    }

    Json::Value body;
    body["ok"] = true;
    callback(HttpResponse::newHttpJsonResponse(body));
}

}  // namespace DeleteFrameHandler
