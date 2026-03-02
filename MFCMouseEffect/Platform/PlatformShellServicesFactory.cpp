#include "pch.h"

#include "Platform/PlatformShellServicesFactory.h"

#include "Platform/PlatformTarget.h"

#if MFX_PLATFORM_WINDOWS
#include "Platform/windows/Shell/Win32ShellServicesFactory.h"
#elif MFX_PLATFORM_MACOS
#include "Platform/macos/Shell/MacosShellServicesFactory.h"
#elif MFX_PLATFORM_LINUX
#include "Platform/linux/Shell/LinuxShellServicesFactory.h"
#endif

namespace mousefx::platform {

ShellPlatformServices CreateShellPlatformServices() {
#if MFX_PLATFORM_WINDOWS
    return windows::CreateWin32ShellPlatformServices();
#elif MFX_PLATFORM_MACOS
    return macos::CreateMacosShellPlatformServices();
#elif MFX_PLATFORM_LINUX
    return linux_shell::CreateLinuxShellPlatformServices();
#else
    return {};
#endif
}

} // namespace mousefx::platform
