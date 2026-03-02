#pragma once

#include <string>

namespace mousefx {
class WebSettingsServer;
}

namespace mousefx::platform {

std::string ReadCoreWebSettingsProbeFilePath();
bool WriteCoreWebSettingsProbeFile(const std::string& filePath, const WebSettingsServer& webSettings);
std::string ReadCoreWebSettingsLaunchProbeFilePath();
bool WriteCoreWebSettingsLaunchProbeFile(
    const std::string& filePath,
    const std::string& settingsUrl,
    bool opened);

} // namespace mousefx::platform
