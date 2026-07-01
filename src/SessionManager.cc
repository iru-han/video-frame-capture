#include "SessionManager.h"

#include <chrono>
#include <cstdio>
#include <iomanip>
#include <random>
#include <regex>
#include <sstream>

namespace SessionManager {

namespace {
std::filesystem::path g_dataRoot;

const std::regex kSessionIdPattern(R"(^([0-9]+)_[0-9a-f]{8}$)");
}  // namespace

void init(const std::filesystem::path &dataRoot) { g_dataRoot = dataRoot; }

std::string newSessionId() {
    auto now = std::chrono::system_clock::now();
    auto ts = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(0, 15);
    static const char *hexDigits = "0123456789abcdef";

    std::ostringstream oss;
    oss << ts << "_";
    for (int i = 0; i < 8; ++i) {
        oss << hexDigits[dist(gen)];
    }
    return oss.str();
}

bool isValidSessionId(const std::string &id) {
    return std::regex_match(id, kSessionIdPattern);
}

std::filesystem::path sessionDirFor(const std::string &id) { return g_dataRoot / id; }

std::filesystem::path framesDirFor(const std::string &id) { return sessionDirFor(id) / "frames"; }

std::filesystem::path videoPathFor(const std::string &id) {
    auto dir = sessionDirFor(id);
    std::error_code ec;
    if (!std::filesystem::exists(dir, ec)) return {};
    for (const auto &entry : std::filesystem::directory_iterator(dir, ec)) {
        if (entry.is_regular_file() && entry.path().stem() == "source") {
            return entry.path();
        }
    }
    return {};
}

void cleanupOldSessions(unsigned maxAgeHours) {
    std::error_code ec;
    if (!std::filesystem::exists(g_dataRoot, ec)) return;

    auto now = std::chrono::duration_cast<std::chrono::seconds>(
                   std::chrono::system_clock::now().time_since_epoch())
                   .count();
    long maxAgeSeconds = static_cast<long>(maxAgeHours) * 3600;

    for (const auto &entry : std::filesystem::directory_iterator(g_dataRoot, ec)) {
        if (!entry.is_directory()) continue;
        std::string name = entry.path().filename().string();
        std::smatch m;
        if (!std::regex_match(name, m, kSessionIdPattern)) continue;

        long ts = std::stol(m[1].str());
        if (now - ts > maxAgeSeconds) {
            std::error_code rmEc;
            std::filesystem::remove_all(entry.path(), rmEc);
        }
    }
}

}  // namespace SessionManager
