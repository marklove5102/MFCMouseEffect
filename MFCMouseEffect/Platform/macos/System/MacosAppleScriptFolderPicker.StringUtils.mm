#include "pch.h"

#include "Platform/macos/System/MacosAppleScriptFolderPicker.ScriptHelpers.h"

#include "MouseFx/Utils/StringUtils.h"

#include <filesystem>
#include <string>

namespace mousefx::platform::macos::apple_script_folder_picker_detail {

std::string EscapeForAppleScriptString(const std::string& value) {
    std::string out;
    out.reserve(value.size());
    for (char c : value) {
        switch (c) {
        case '\\':
            out += "\\\\";
            break;
        case '"':
            out += "\\\"";
            break;
        case '\r':
        case '\n':
            out.push_back(' ');
            break;
        default:
            out.push_back(c);
            break;
        }
    }
    return out;
}

std::string TrimAsciiWhitespace(std::string value) {
    const auto isSpace = [](char ch) {
        return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
    };
    size_t begin = 0;
    while (begin < value.size() && isSpace(value[begin])) {
        ++begin;
    }
    if (begin >= value.size()) {
        return {};
    }
    size_t end = value.size();
    while (end > begin && isSpace(value[end - 1])) {
        --end;
    }
    return value.substr(begin, end - begin);
}

bool StartsWith(const std::string& value, const char* prefix) {
    if (!prefix) {
        return false;
    }
    const std::string marker(prefix);
    if (value.size() < marker.size()) {
        return false;
    }
    return value.compare(0, marker.size(), marker) == 0;
}

std::string NormalizeInitialDirectoryForAppleScript(const std::wstring& initialPath) {
    if (initialPath.empty()) {
        return {};
    }
    std::error_code ec;
    const std::filesystem::path path(initialPath);
    if (!std::filesystem::exists(path, ec) || ec || !std::filesystem::is_directory(path, ec) || ec) {
        return {};
    }
    return Utf16ToUtf8(path.wstring().c_str());
}

} // namespace mousefx::platform::macos::apple_script_folder_picker_detail
