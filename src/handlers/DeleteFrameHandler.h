#pragma once
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>

#include <functional>

namespace DeleteFrameHandler {

void handle(const drogon::HttpRequestPtr &req,
            std::function<void(const drogon::HttpResponsePtr &)> &&callback);

}  // namespace DeleteFrameHandler
