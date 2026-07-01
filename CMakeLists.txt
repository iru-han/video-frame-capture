cmake_minimum_required(VERSION 3.16)
project(video_frame_capture CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Not REQUIRED here on purpose: the packaged DrogonConfig.cmake propagates
# REQUIRED to *every* nested find_dependency() call (pg/sqlite3/mysql/
# hiredis/coz-profiler/...) whenever our own find_package(Drogon) is
# REQUIRED, even for optional features baked into the prebuilt .so whose
# -dev headers aren't installed on this machine (their runtime .so is
# still resolvable via the dynamic linker, so the build/link works fine
# without the -dev package). We check Drogon_FOUND ourselves below instead.
find_package(Drogon CONFIG)
if(NOT Drogon_FOUND)
    message(FATAL_ERROR "Drogon not found - install libdrogon-dev")
endif()

add_executable(video_frame_capture
    src/main.cc
    src/ProcessRunner.cc
    src/SessionManager.cc
    src/handlers/UploadHandler.cc
    src/handlers/ExtractHandler.cc
    src/handlers/DeleteFrameHandler.cc
    src/handlers/LabelHandler.cc
    src/handlers/ImageUploadHandler.cc
)

target_link_libraries(video_frame_capture PRIVATE Drogon::Drogon)
