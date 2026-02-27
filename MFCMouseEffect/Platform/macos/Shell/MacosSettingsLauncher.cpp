#include "Platform/macos/Shell/MacosSettingsLauncher.h"
#include "Platform/macos/Shell/MacosSettingsLauncherSwiftBridge.h"

#include "Platform/posix/Shell/PosixSettingsLauncher.h"
#include "Platform/posix/Shell/PosixSettingsLauncher.Internal.h"

namespace mousefx {

bool MacosSettingsLauncher::OpenUrlUtf8(const std::string& url) {
    // Keep capture-mode behavior unchanged for existing regression contracts.
    if (!ReadLaunchCaptureFilePath().empty()) {
        return LaunchUrlWithPosixCommand("open", url);
    }
    if (!IsLaunchInputValid(url)) {
        return false;
    }
    if (mfx_macos_open_settings_url(url.c_str())) {
        return true;
    }
    return LaunchUrlWithPosixCommand("open", url);
}

} // namespace mousefx
