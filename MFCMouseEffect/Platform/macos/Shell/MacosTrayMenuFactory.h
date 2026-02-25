#pragma once

#include "MouseFx/Core/Shell/IAppShellHost.h"
#include "Platform/macos/Shell/MacosTrayMenuLocalization.h"

#if defined(__APPLE__)
#import <AppKit/AppKit.h>
#endif

namespace mousefx::macos_tray {

#if defined(__APPLE__)
struct MacosTrayMenuObjects final {
    NSStatusItem* statusItem = nil;
    NSMenu* menu = nil;
    id actionBridge = nil;
};

bool BuildMacosTrayMenu(
    IAppShellHost* host,
    const MacosTrayMenuText& menuText,
    MacosTrayMenuObjects* outObjects);
void ReleaseMacosTrayMenu(MacosTrayMenuObjects* objects);
#endif

} // namespace mousefx::macos_tray
