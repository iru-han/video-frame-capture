#pragma once
#include <filesystem>
#include <string>

namespace SessionManager {

// Absolute path to the data root directory (set once at startup).
void init(const std::filesystem::path &dataRoot);

// Generates a new session id of the form "<unix_ts>_<8hex>".
std::string newSessionId();

// Validates a session id matches "^[0-9]+_[0-9a-f]{8}$".
bool isValidSessionId(const std::string &id);

std::filesystem::path sessionDirFor(const std::string &id);
std::filesystem::path framesDirFor(const std::string &id);
std::filesystem::path labelsDirFor(const std::string &id);
std::filesystem::path classesFileFor(const std::string &id);
// Returns the video file path for a session, regardless of its extension
// (looks for a file named "source.*" inside the session dir). Empty path if none found.
std::filesystem::path videoPathFor(const std::string &id);

// Deletes session directories whose embedded timestamp is older than maxAgeHours.
void cleanupOldSessions(unsigned maxAgeHours);

}  // namespace SessionManager
