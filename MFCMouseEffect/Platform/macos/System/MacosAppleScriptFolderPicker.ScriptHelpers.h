#pragma once

#include "Platform/PlatformNativeFolderPicker.h"

#include <string>

namespace mousefx::platform::macos::apple_script_folder_picker_detail {

std::string EscapeForAppleScriptString(const std::string& value);
std::string TrimAsciiWhitespace(std::string value);
bool StartsWith(const std::string& value, const char* prefix);
std::string NormalizeInitialDirectoryForAppleScript(const std::wstring& initialPath);

std::string BuildAppleScriptChooseFolderSource(
    const std::wstring& title,
    const std::wstring& initialPath);

NativeFolderPickResult ExecuteAppleScriptChooseFolderOnMainThread(
    const std::wstring& title,
    const std::wstring& initialPath);

} // namespace mousefx::platform::macos::apple_script_folder_picker_detail
