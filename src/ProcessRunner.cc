#include "ProcessRunner.h"

#include <sys/wait.h>

#include <array>
#include <cstdio>
#include <sstream>

namespace ProcessRunner {

namespace {

// Wraps a path in single quotes for safe use in a /bin/sh -c command,
// escaping any embedded single quotes. Callers only ever pass
// server-controlled paths (built from validated session ids / fixed
// filenames), never raw user input, but this is defense in depth.
std::string shellQuote(const std::string &s) {
    std::string out = "'";
    for (char c : s) {
        if (c == '\'') {
            out += "'\\''";
        } else {
            out += c;
        }
    }
    out += "'";
    return out;
}

struct Result {
    int exitCode;
    std::string stdoutText;
};

Result run(const std::string &cmd) {
    Result result{-1, ""};
    std::array<char, 4096> buffer{};
    FILE *pipe = popen(cmd.c_str(), "r");
    if (!pipe) return result;

    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        result.stdoutText += buffer.data();
    }
    int status = pclose(pipe);
    result.exitCode = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    return result;
}

}  // namespace

double getDurationSeconds(const std::filesystem::path &videoPath) {
    std::ostringstream cmd;
    cmd << "ffprobe -v error -show_entries format=duration -of csv=p=0 "
        << shellQuote(videoPath.string()) << " 2>/dev/null";

    Result r = run(cmd.str());
    if (r.exitCode != 0 || r.stdoutText.empty()) return -1.0;

    try {
        return std::stod(r.stdoutText);
    } catch (...) {
        return -1.0;
    }
}

bool extractFrames(const std::filesystem::path &videoPath,
                    const std::filesystem::path &framesDir,
                    double intervalSeconds) {
    if (intervalSeconds < 0.1) return false;

    std::ostringstream cmd;
    cmd << "ffmpeg -y -i " << shellQuote(videoPath.string())
        << " -vf " << shellQuote("fps=1/" + std::to_string(intervalSeconds))
        << " -q:v 2 " << shellQuote((framesDir / "frame_%04d.jpg").string())
        << " 2>/dev/null";

    Result r = run(cmd.str());
    return r.exitCode == 0;
}

}  // namespace ProcessRunner
