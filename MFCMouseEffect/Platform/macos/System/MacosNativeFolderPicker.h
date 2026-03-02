#pragma once

#include "Platform/PlatformNativeFolderPicker.h"

#include <string>

namespace mousefx::platform::macos {

class MacosNativeFolderPicker final {
public:
    static NativeFolderPickResult PickFolder(
        const std::wstring& title,
        const std::wstring& initialPath);
};

} // namespace mousefx::platform::macos
