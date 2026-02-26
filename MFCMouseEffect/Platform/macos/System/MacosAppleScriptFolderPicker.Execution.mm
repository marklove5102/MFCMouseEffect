#include "pch.h"

#include "Platform/macos/System/MacosAppleScriptFolderPicker.ScriptHelpers.h"

#if defined(__APPLE__)
#import <Foundation/Foundation.h>
#endif

#include "MouseFx/Utils/StringUtils.h"

#include <string>

namespace mousefx::platform::macos::apple_script_folder_picker_detail {

namespace {

std::string NSStringToUtf8String(NSString* text) {
    if (text == nil || text.length == 0) {
        return {};
    }
    const char* raw = [text UTF8String];
    if (raw == nullptr || raw[0] == '\0') {
        return {};
    }
    return std::string(raw);
}

std::string ReadAppleScriptError(NSDictionary* errorInfo) {
    if (errorInfo == nil || errorInfo.count == 0) {
        return "apple_script_execute_failed";
    }
    NSString* message = errorInfo[NSAppleScriptErrorMessage];
    NSNumber* number = errorInfo[NSAppleScriptErrorNumber];
    std::string out;
    if (number != nil) {
        out += "code=" + std::to_string([number integerValue]);
    }
    if (message != nil && message.length > 0) {
        if (!out.empty()) {
            out += ",";
        }
        out += NSStringToUtf8String(message);
    }
    if (out.empty()) {
        return "apple_script_execute_failed";
    }
    return out;
}

} // namespace

std::string BuildAppleScriptChooseFolderSource(
    const std::wstring& title,
    const std::wstring& initialPath) {
    const std::string safeTitle = EscapeForAppleScriptString(
        Utf16ToUtf8(title.empty() ? L"Select WASM plugin folder" : title.c_str()));
    const std::string safeInitialDir = EscapeForAppleScriptString(
        NormalizeInitialDirectoryForAppleScript(initialPath));

    std::string source = "try\n";
    std::string chooseLine = "set pickedFolder to choose folder with prompt \"" + safeTitle + "\"";
    if (!safeInitialDir.empty()) {
        chooseLine += " default location ((POSIX file \"" + safeInitialDir + "\") as alias)";
    }
    source += chooseLine;
    source += "\nreturn POSIX path of pickedFolder\n";
    source += "on error number -128\n";
    source += "return \"__MFX_CANCELLED__\"\n";
    source += "on error errMsg number errNum\n";
    source += "return \"__MFX_ERROR__\" & errNum & \":\" & errMsg\n";
    source += "end try\n";
    return source;
}

NativeFolderPickResult ExecuteAppleScriptChooseFolderOnMainThread(
    const std::wstring& title,
    const std::wstring& initialPath) {
#if !defined(__APPLE__)
    (void)title;
    (void)initialPath;
    NativeFolderPickResult out{};
    out.error = "native_folder_picker_not_supported";
    return out;
#else
    NativeFolderPickResult out{};
    const std::string sourceUtf8 = BuildAppleScriptChooseFolderSource(title, initialPath);
    NSString* source = [NSString stringWithUTF8String:sourceUtf8.c_str()];
    if (source == nil || source.length == 0) {
        out.error = "apple_script_source_invalid";
        return out;
    }

    NSAppleScript* script = [[NSAppleScript alloc] initWithSource:source];
    if (script == nil) {
        out.error = "apple_script_compile_failed";
        return out;
    }

    NSDictionary* errorInfo = nil;
    NSAppleEventDescriptor* result = [script executeAndReturnError:&errorInfo];
    [script release];
    if (result == nil) {
        out.error = ReadAppleScriptError(errorInfo);
        return out;
    }

    NSString* valueText = [result stringValue];
    const std::string trimmed = TrimAsciiWhitespace(NSStringToUtf8String(valueText));
    if (trimmed == "__MFX_CANCELLED__") {
        out.cancelled = true;
        out.error = "cancelled";
        return out;
    }
    if (StartsWith(trimmed, "__MFX_ERROR__")) {
        out.error = trimmed.empty() ? "apple_script_choose_folder_failed" : trimmed;
        return out;
    }
    if (trimmed.empty()) {
        out.error = "selected folder path missing";
        return out;
    }

    out.ok = true;
    out.folderPath = Utf8ToWString(trimmed.c_str());
    if (out.folderPath.empty()) {
        out.ok = false;
        out.error = "selected folder path missing";
    }
    return out;
#endif
}

} // namespace mousefx::platform::macos::apple_script_folder_picker_detail
