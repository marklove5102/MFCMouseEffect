#include "pch.h"

#include "Platform/macos/Shell/MacosUserNotificationService.Internal.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <system_error>

namespace mousefx::macos_notification_detail {
namespace {

constexpr std::string_view kNotificationCaptureFileEnv = "MFX_TEST_NOTIFICATION_CAPTURE_FILE";

bool EnsureParentDirectory(const std::filesystem::path& filePath) {
    const std::filesystem::path parentPath = filePath.parent_path();
    if (parentPath.empty()) {
        return true;
    }
    std::error_code ec;
    std::filesystem::create_directories(parentPath, ec);
    return !ec;
}

} // namespace

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

} // namespace mousefx::macos_notification_detail
