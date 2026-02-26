#include "pch.h"

#include "Platform/macos/System/MacosAppleScriptFolderPicker.Internal.h"
#include "Platform/macos/System/MacosAppleScriptFolderPicker.ScriptHelpers.h"

#if defined(__APPLE__)
#import <Foundation/Foundation.h>
#import <dispatch/dispatch.h>
#endif

namespace mousefx::platform::macos::apple_script_folder_picker_detail {

NativeFolderPickResult PickFolderViaAppleScriptInternal(
    const std::wstring& title,
    const std::wstring& initialPath) {
#if defined(__APPLE__)
    if ([NSThread isMainThread]) {
        return ExecuteAppleScriptChooseFolderOnMainThread(title, initialPath);
    }
    __block NativeFolderPickResult out{};
    dispatch_sync(dispatch_get_main_queue(), ^{
        out = ExecuteAppleScriptChooseFolderOnMainThread(title, initialPath);
    });
    return out;
#else
    (void)title;
    (void)initialPath;
    NativeFolderPickResult out{};
    out.error = "native_folder_picker_not_supported";
    return out;
#endif
}

} // namespace mousefx::platform::macos::apple_script_folder_picker_detail
