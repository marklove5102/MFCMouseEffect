#include "pch.h"

#include "Platform/windows/Shell/Win32ShellServicesFactory.h"

#include "Platform/windows/Shell/Win32DpiAwarenessService.h"
#include "Platform/windows/Shell/Win32EventLoopService.h"
#include "Platform/windows/Shell/Win32SettingsLauncher.h"
#include "Platform/windows/Shell/Win32SingleInstanceGuard.h"
#include "Platform/windows/Shell/Win32TrayService.h"
#include "Platform/windows/Shell/Win32UserNotificationService.h"

namespace mousefx::platform::windows {

ShellPlatformServices CreateWin32ShellPlatformServices() {
    ShellPlatformServices services{};
    services.trayService = std::make_unique<Win32TrayService>();
    services.settingsLauncher = std::make_unique<Win32SettingsLauncher>();
    services.singleInstanceGuard = std::make_unique<Win32SingleInstanceGuard>();
    services.dpiAwarenessService = std::make_unique<Win32DpiAwarenessService>();
    services.eventLoopService = std::make_unique<Win32EventLoopService>();
    services.notifier = std::make_unique<Win32UserNotificationService>();
    return services;
}

} // namespace mousefx::platform::windows
