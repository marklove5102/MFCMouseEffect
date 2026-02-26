#include "Platform/posix/Shell/PosixSettingsLauncher.Internal.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string_view>
#include <system_error>

namespace mousefx {
namespace {

constexpr std::string_view kLaunchCaptureFileEnv = "MFX_TEST_SETTINGS_LAUNCH_CAPTURE_FILE";

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

bool IsLaunchInputValid(std::string_view url) {
    if (url.empty()) {
        return false;
    }

    for (unsigned char c : url) {
        if (c < 0x20 || c == 0x7f) {
            return false;
        }
    }

    return true;
}

std::string ReadLaunchCaptureFilePath() {
    const char* filePath = std::getenv(kLaunchCaptureFileEnv.data());
    if (filePath == nullptr || filePath[0] == '\0') {
        return {};
    }
    return filePath;
}

bool WriteLaunchCaptureFile(const std::string& filePath, const char* command, const std::string& url) {
    if (filePath.empty() || command == nullptr || command[0] == '\0') {
        return false;
    }

    const std::filesystem::path targetPath(filePath);
    if (!EnsureParentDirectory(targetPath)) {
        return false;
    }

    const std::string tmpPath = filePath + ".tmp";
    {
        std::ofstream out(tmpPath, std::ios::out | std::ios::trunc);
        if (!out.is_open()) {
            return false;
        }
        out << "command=" << command << '\n';
        out << "url=" << url << '\n';
        out << "captured=1\n";
        out.flush();
        if (!out.good()) {
            return false;
        }
    }

    std::error_code ec;
    std::filesystem::rename(tmpPath, targetPath, ec);
    if (!ec) {
        return true;
    }

    std::filesystem::remove(targetPath, ec);
    ec.clear();
    std::filesystem::rename(tmpPath, targetPath, ec);
    return !ec;
}

} // namespace mousefx
