#include "pch.h"

#include "Platform/macos/System/MacosNativeFolderPicker.h"
#include "Platform/macos/System/MacosAppleScriptFolderPicker.h"
#include "Platform/macos/System/MacosOpenPanelFolderPicker.h"

namespace mousefx::platform::macos {

NativeFolderPickResult MacosNativeFolderPicker::PickFolder(
    const std::wstring& title,
    const std::wstring& initialPath) {
#if defined(__APPLE__)
    const NativeFolderPickResult scriptResult = PickFolderViaAppleScript(title, initialPath);
    if (scriptResult.ok || scriptResult.cancelled) {
        return scriptResult;
    }
    NativeFolderPickResult panelResult = PickFolderViaOpenPanel(title, initialPath);
    if (!scriptResult.error.empty() && !panelResult.ok && !panelResult.cancelled) {
        if (!panelResult.error.empty()) {
            panelResult.error += "; ";
        }
        panelResult.error += "fallback_from_apple_script: " + scriptResult.error;
    }
    return panelResult;
#else
    (void)title;
    (void)initialPath;
    NativeFolderPickResult out{};
    out.error = "native_folder_picker_not_supported";
    return out;
#endif
}

} // namespace mousefx::platform::macos
