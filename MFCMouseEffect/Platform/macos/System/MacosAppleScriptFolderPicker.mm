#include "pch.h"

#include "Platform/macos/System/MacosAppleScriptFolderPicker.h"
#include "Platform/macos/System/MacosAppleScriptFolderPicker.Internal.h"

namespace mousefx::platform::macos {

NativeFolderPickResult PickFolderViaAppleScript(
    const std::wstring& title,
    const std::wstring& initialPath) {
    return apple_script_folder_picker_detail::PickFolderViaAppleScriptInternal(title, initialPath);
}

} // namespace mousefx::platform::macos
