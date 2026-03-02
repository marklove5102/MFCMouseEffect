#pragma once

#include "MouseFx/Core/Shell/IAppShellHost.h"
#include "MouseFx/Core/Shell/ShellPlatformServices.h"
#include "Platform/IPlatformAppShell.h"
#include "Platform/PlatformTarget.h"
#include "Platform/posix/Shell/ScaffoldSettingsRuntime.h"

namespace mousefx::platform {

#if MFX_PLATFORM_MACOS || MFX_PLATFORM_LINUX
class PosixScaffoldAppShell final : public IPlatformAppShell, private IAppShellHost {
public:
    explicit PosixScaffoldAppShell(ShellPlatformServices services);

    bool Initialize(const AppShellStartOptions& options) override;
    int RunMessageLoop() override;
    void Shutdown() override;

private:
    AppController* AppControllerForShell() noexcept override;
    void OpenSettingsFromShell() override;
    void RequestExitFromShell() override;

    void StartStdinExitMonitor();

private:
    ShellPlatformServices services_{};
    ScaffoldSettingsRuntime scaffoldRuntime_{};
    bool initialized_ = false;
    bool backgroundMode_ = false;
    bool stdinMonitorStarted_ = false;
};
#endif

} // namespace mousefx::platform
