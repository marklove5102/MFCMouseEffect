#include "Platform/macos/Shell/MacosUserNotificationService.h"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <system_error>

namespace mousefx {

namespace {

constexpr std::string_view kNotificationCaptureFileEnv = "MFX_TEST_NOTIFICATION_CAPTURE_FILE";

std::string EscapeForAppleScriptString(const std::string& value) {
    std::string out;
    out.reserve(value.size());
    for (char c : value) {
        switch (c) {
        case '\\':
            out += "\\\\";
            break;
        case '"':
            out += "\\\"";
            break;
        case '\r':
        case '\n':
            out.push_back(' ');
            break;
        default:
            out.push_back(c);
            break;
        }
    }
    return out;
}

std::string ShellSingleQuote(const std::string& value) {
    std::string out;
    out.reserve(value.size() + 8);
    out.push_back('\'');
    for (char c : value) {
        if (c == '\'') {
            out += "'\\''";
        } else {
            out.push_back(c);
        }
    }
    out.push_back('\'');
    return out;
}

bool EnsureParentDirectory(const std::filesystem::path& filePath) {
    const std::filesystem::path parentPath = filePath.parent_path();
    if (parentPath.empty()) {
        return true;
    }
    std::error_code ec;
    std::filesystem::create_directories(parentPath, ec);
    return !ec;
}

void AppendTestNotificationCapture(const std::string& titleUtf8, const std::string& messageUtf8) {
    const char* rawPath = std::getenv(kNotificationCaptureFileEnv.data());
    if (rawPath == nullptr || rawPath[0] == '\0') {
        return;
    }

    const std::filesystem::path filePath(rawPath);
    if (!EnsureParentDirectory(filePath)) {
        return;
    }

    std::ofstream out(filePath, std::ios::out | std::ios::app);
    if (!out.is_open()) {
        return;
    }
    out << "title=" << titleUtf8 << '\t' << "message=" << messageUtf8 << '\n';
}

} // namespace

void MacosUserNotificationService::ShowWarning(
    const std::string& titleUtf8,
    const std::string& messageUtf8) {
    const std::string safeTitle = EscapeForAppleScriptString(
        titleUtf8.empty() ? "MFCMouseEffect" : titleUtf8);
    const std::string safeMessage = EscapeForAppleScriptString(
        messageUtf8.empty() ? "(empty)" : messageUtf8);
    AppendTestNotificationCapture(safeTitle, safeMessage);

    const std::string script =
        "display notification \"" + safeMessage + "\" with title \"" + safeTitle + "\"";
    const std::string command =
        "osascript -e " + ShellSingleQuote(script) + " >/dev/null 2>&1";

    if (std::system(command.c_str()) == 0) {
        return;
    }

    std::fprintf(stderr, "[mousefx][warn] %s: %s\n", safeTitle.c_str(), safeMessage.c_str());
}

} // namespace mousefx
