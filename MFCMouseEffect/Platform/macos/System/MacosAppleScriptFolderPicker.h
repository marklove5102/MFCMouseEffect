#pragma once

#include "Platform/PlatformNativeFolderPicker.h"

#include <string>

namespace mousefx::platform::macos {

NativeFolderPickResult PickFolderViaAppleScript(
    const std::wstring& title,
    const std::wstring& initialPath);

} // namespace mousefx::platform::macos
