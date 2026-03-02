#include "Platform/macos/Shell/MacosSettingsLauncher.h"

#include "Platform/posix/Shell/PosixSettingsLauncher.h"

namespace mousefx {

bool MacosSettingsLauncher::OpenUrlUtf8(const std::string& url) {
    return LaunchUrlWithPosixCommand("open", url);
}

} // namespace mousefx
