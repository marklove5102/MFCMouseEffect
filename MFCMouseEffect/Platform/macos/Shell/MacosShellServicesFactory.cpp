#include "pch.h"

#include "Platform/macos/Shell/MacosShellServicesFactory.h"

#include "Platform/macos/Shell/MacosDpiAwarenessService.h"
#include "Platform/macos/Shell/MacosEventLoopService.h"
#include "Platform/macos/Shell/MacosSettingsLauncher.h"
#include "Platform/macos/Shell/MacosSingleInstanceGuard.h"
#include "Platform/macos/Shell/MacosTrayService.h"
#include "Platform/macos/Shell/MacosUserNotificationService.h"

namespace mousefx::platform::macos {

ShellPlatformServices CreateMacosShellPlatformServices() {
    ShellPlatformServices services{};
    services.trayService = std::make_unique<MacosTrayService>();
    services.settingsLauncher = std::make_unique<MacosSettingsLauncher>();
    services.singleInstanceGuard = std::make_unique<MacosSingleInstanceGuard>();
    services.dpiAwarenessService = std::make_unique<MacosDpiAwarenessService>();
    services.eventLoopService = std::make_unique<MacosEventLoopService>();
    services.notifier = std::make_unique<MacosUserNotificationService>();
    return services;
}

} // namespace mousefx::platform::macos
