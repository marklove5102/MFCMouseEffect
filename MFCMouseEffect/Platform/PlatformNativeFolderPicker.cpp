#include "pch.h"

#include "Platform/PlatformNativeFolderPicker.h"

#if defined(_WIN32)
#include "Platform/windows/System/Win32NativeFolderPicker.h"
#elif defined(__APPLE__)
#include "Platform/macos/System/MacosNativeFolderPicker.h"
#endif

namespace mousefx::platform {

NativeFolderPickResult PickFolder(const std::wstring& title, const std::wstring& initialPath) {
#if defined(_WIN32)
    return windows::Win32NativeFolderPicker::PickFolder(title, initialPath);
#elif defined(__APPLE__)
    return macos::MacosNativeFolderPicker::PickFolder(title, initialPath);
#else
    NativeFolderPickResult result{};
    result.ok = false;
    result.cancelled = false;
    result.error = "native_folder_picker_not_supported";
    return result;
#endif
}

bool IsNativeFolderPickerSupported() {
#if defined(_WIN32) || defined(__APPLE__)
    return true;
#else
    return false;
#endif
}

} // namespace mousefx::platform
