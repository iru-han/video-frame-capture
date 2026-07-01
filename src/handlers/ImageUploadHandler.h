#pragma once
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>

#include <functional>

namespace ImageUploadHandler {

// Accepts one or more image files (multipart, any field name) and creates a new
// session containing them as frames, skipping the video/extraction step entirely.
void handle(const drogon::HttpRequestPtr &req,
            std::function<void(const drogon::HttpResponsePtr &)> &&callback);

}  // namespace ImageUploadHandler
