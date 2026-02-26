#pragma once

#include "Platform/PlatformNativeFolderPicker.h"

#include <string>

namespace mousefx::platform::macos::apple_script_folder_picker_detail {

NativeFolderPickResult PickFolderViaAppleScriptInternal(
    const std::wstring& title,
    const std::wstring& initialPath);

} // namespace mousefx::platform::macos::apple_script_folder_picker_detail
