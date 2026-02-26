#include "Platform/posix/Shell/PosixSettingsLauncher.h"
#include "Platform/posix/Shell/PosixSettingsLauncher.Internal.h"

namespace mousefx {

bool LaunchUrlWithPosixCommand(const char* command, const std::string& url) {
    if (command == nullptr || command[0] == '\0') {
        return false;
    }
    if (!IsLaunchInputValid(url)) {
        return false;
    }

    const std::string captureFilePath = ReadLaunchCaptureFilePath();
    if (!captureFilePath.empty()) {
        return WriteLaunchCaptureFile(captureFilePath, command, url);
    }
    return SpawnSettingsCommand(command, url);
}

} // namespace mousefx
