#include "Platform/linux/Shell/LinuxSettingsLauncher.h"

#include "Platform/posix/Shell/PosixSettingsLauncher.h"

namespace mousefx {

bool LinuxSettingsLauncher::OpenUrlUtf8(const std::string& url) {
    return LaunchUrlWithPosixCommand("xdg-open", url);
}

bool LinuxSettingsLauncher::OpenApplicationPathUtf8(const std::string& appPath) {
    return LaunchAppWithPosixCommand("xdg-open", appPath);
}

} // namespace mousefx
