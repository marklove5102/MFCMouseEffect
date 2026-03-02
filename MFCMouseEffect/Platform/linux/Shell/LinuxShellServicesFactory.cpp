#include "pch.h"

#include "Platform/linux/Shell/LinuxShellServicesFactory.h"

#include "Platform/linux/Shell/LinuxEventLoopService.h"
#include "Platform/linux/Shell/LinuxSettingsLauncher.h"
#include "Platform/linux/Shell/LinuxSingleInstanceGuard.h"
#include "Platform/linux/Shell/LinuxTrayService.h"
#include "Platform/linux/Shell/LinuxUserNotificationService.h"

namespace mousefx::platform::linux_shell {

ShellPlatformServices CreateLinuxShellPlatformServices() {
    ShellPlatformServices services{};
    services.trayService = std::make_unique<LinuxTrayService>();
    services.settingsLauncher = std::make_unique<LinuxSettingsLauncher>();
    services.singleInstanceGuard = std::make_unique<LinuxSingleInstanceGuard>();
    services.eventLoopService = std::make_unique<LinuxEventLoopService>();
    services.notifier = std::make_unique<LinuxUserNotificationService>();
    return services;
}

} // namespace mousefx::platform::linux_shell
