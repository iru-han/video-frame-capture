#pragma once
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>

#include <functional>

namespace LabelHandler {

// GET /api/classes?session_id=... -> { "classes": ["leg", "foot", ...] }
void getClasses(const drogon::HttpRequestPtr &req,
                 std::function<void(const drogon::HttpResponsePtr &)> &&callback);

// POST /api/classes { session_id, class } -> { "classes": [...] }
void postClass(const drogon::HttpRequestPtr &req,
                std::function<void(const drogon::HttpResponsePtr &)> &&callback);

// GET /api/label?session_id=...&filename=... -> { "boxes": [ { class, x_center, y_center,
// width, height }, ... ] }
void getLabel(const drogon::HttpRequestPtr &req,
              std::function<void(const drogon::HttpResponsePtr &)> &&callback);

// GET /api/labels?session_id=... -> { "labeled": ["frame_0001", ...] } (frame filename stems
// that already have a saved label file, regardless of box count)
void getLabeledFrames(const drogon::HttpRequestPtr &req,
                       std::function<void(const drogon::HttpResponsePtr &)> &&callback);

// POST /api/label { session_id, filename, boxes: [ { class, x_center, y_center, width,
// height }, ... ] } -> { "ok": true }
void postLabel(const drogon::HttpRequestPtr &req,
               std::function<void(const drogon::HttpResponsePtr &)> &&callback);

}  // namespace LabelHandler
