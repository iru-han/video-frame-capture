#pragma once
#include <filesystem>
#include <string>

namespace ProcessRunner {

// Returns the video duration in seconds via ffprobe, or -1.0 on failure
// (missing file, corrupt video, ffprobe not found, unparsable output).
double getDurationSeconds(const std::filesystem::path &videoPath);

// Extracts one frame every `intervalSeconds` seconds into framesDir/frame_%04d.jpg.
// Returns true iff ffmpeg exits with status 0.
bool extractFrames(const std::filesystem::path &videoPath,
                    const std::filesystem::path &framesDir,
                    double intervalSeconds);

}  // namespace ProcessRunner
