#pragma once

#include "MouseFx/Core/Control/AppController.h"
#include "MouseFx/Core/Shell/IAppShellHost.h"
#include "MouseFx/Core/Shell/ShellPlatformServices.h"
#include "MouseFx/Server/WebSettingsServer.h"
#include "Platform/IPlatformAppShell.h"
#include "Platform/PlatformTarget.h"

#include <functional>
#include <memory>

namespace mousefx::platform {

#if MFX_PLATFORM_MACOS || MFX_PLATFORM_LINUX
class PosixCoreAppShell final : public IPlatformAppShell, private IAppShellHost {
public:
    explicit PosixCoreAppShell(ShellPlatformServices services);

    bool Initialize(const AppShellStartOptions& options) override;
    int RunMessageLoop() override;
    void Shutdown() override;

private:
    AppController* AppControllerForShell() noexcept override;
    void OpenSettingsFromShell() override;
    void RequestExitFromShell() override;

    bool PostShellTask(std::function<void()> task);
    void RequestExitOnLoop();
    void ShowWebSettings();
    bool SetupRegressionWebSettingsProbe();
    void StartStdinExitMonitor();

private:
    ShellPlatformServices services_{};
    std::unique_ptr<AppController> appController_{};
    std::unique_ptr<WebSettingsServer> webSettings_{};
    bool initialized_ = false;
    bool backgroundMode_ = false;
    bool stdinMonitorStarted_ = false;
};
#endif

} // namespace mousefx::platform
