#include "pch.h"

#include "Platform/posix/Shell/PosixCoreWebSettingsProbe.h"

#include "MouseFx/Server/WebSettingsServer.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <utility>
#include <string_view>
#include <system_error>
#include <vector>

namespace mousefx::platform {
namespace {

constexpr std::string_view kProbeFileEnv = "MFX_CORE_WEB_SETTINGS_PROBE_FILE";
constexpr std::string_view kLaunchProbeFileEnv = "MFX_CORE_WEB_SETTINGS_LAUNCH_PROBE_FILE";

bool EnsureParentDirectory(const std::filesystem::path& filePath) {
    const std::filesystem::path parentPath = filePath.parent_path();
    if (parentPath.empty()) {
        return true;
    }
    std::error_code ec;
    std::filesystem::create_directories(parentPath, ec);
    return !ec;
}

std::string ReadProbeFilePathFromEnv(std::string_view envKey) {
    const char* filePath = std::getenv(envKey.data());
    if (filePath == nullptr || filePath[0] == '\0') {
        return {};
    }
    return filePath;
}

bool WriteProbeFile(
    const std::string& filePath,
    const std::vector<std::pair<std::string, std::string>>& lines) {
    if (filePath.empty() || lines.empty()) {
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

        for (const auto& [key, value] : lines) {
            out << key << "=" << value << '\n';
        }
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

} // namespace

std::string ReadCoreWebSettingsProbeFilePath() {
    return ReadProbeFilePathFromEnv(kProbeFileEnv);
}

bool WriteCoreWebSettingsProbeFile(const std::string& filePath, const WebSettingsServer& webSettings) {
    const std::string token = webSettings.Token();
    const std::string url = webSettings.Url();
    return WriteProbeFile(filePath, {
                                   {"url", url},
                                   {"token", token},
                                   {"port", std::to_string(webSettings.Port())},
                               });
}

std::string ReadCoreWebSettingsLaunchProbeFilePath() {
    return ReadProbeFilePathFromEnv(kLaunchProbeFileEnv);
}

bool WriteCoreWebSettingsLaunchProbeFile(
    const std::string& filePath,
    const std::string& settingsUrl,
    bool opened) {
    return WriteProbeFile(filePath, {
                                   {"url", settingsUrl},
                                   {"opened", opened ? "1" : "0"},
                               });
}

} // namespace mousefx::platform
