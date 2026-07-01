#include <drogon/HttpAppFramework.h>
#include <drogon/drogon.h>

#include <filesystem>

#include "SessionManager.h"
#include "handlers/DeleteFrameHandler.h"
#include "handlers/ExtractHandler.h"
#include "handlers/ImageUploadHandler.h"
#include "handlers/LabelHandler.h"
#include "handlers/UploadHandler.h"

using namespace drogon;

int main() {
    auto projectRoot = std::filesystem::current_path().parent_path();
    auto dataRoot = projectRoot / "data";
    auto frontendRoot = projectRoot / "frontend";

    std::filesystem::create_directories(dataRoot);

    SessionManager::init(dataRoot);
    SessionManager::cleanupOldSessions(/*maxAgeHours=*/24);

    app().setDocumentRoot(frontendRoot.string());
    app().addListener("0.0.0.0", 8848);
    app().setClientMaxBodySize(2ULL * 1024 * 1024 * 1024);  // 2GB, default is 1MB

    app().addALocation("/data", "", dataRoot.string(),
                        /*isCaseSensitive=*/false,
                        /*allowAll=*/true,
                        /*isRecursive=*/true);

    app().registerHandler(
        "/api/upload",
        [](const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) {
            UploadHandler::handle(req, std::move(callback));
        },
        {Post});

    app().registerHandler(
        "/api/extract",
        [](const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) {
            ExtractHandler::handle(req, std::move(callback));
        },
        {Post});

    app().registerHandler(
        "/api/frame",
        [](const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) {
            DeleteFrameHandler::handle(req, std::move(callback));
        },
        {Delete});

    app().registerHandler(
        "/api/classes",
        [](const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) {
            LabelHandler::getClasses(req, std::move(callback));
        },
        {Get});

    app().registerHandler(
        "/api/classes",
        [](const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) {
            LabelHandler::postClass(req, std::move(callback));
        },
        {Post});

    app().registerHandler(
        "/api/label",
        [](const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) {
            LabelHandler::getLabel(req, std::move(callback));
        },
        {Get});

    app().registerHandler(
        "/api/label",
        [](const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) {
            LabelHandler::postLabel(req, std::move(callback));
        },
        {Post});

    app().registerHandler(
        "/api/labels",
        [](const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) {
            LabelHandler::getLabeledFrames(req, std::move(callback));
        },
        {Get});

    app().registerHandler(
        "/api/upload-images",
        [](const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) {
            ImageUploadHandler::handle(req, std::move(callback));
        },
        {Post});

    app().run();
    return 0;
}
