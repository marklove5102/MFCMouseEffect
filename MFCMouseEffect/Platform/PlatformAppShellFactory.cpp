#include "pch.h"

#include "Platform/PlatformAppShellFactory.h"

#include <utility>

#include "Platform/PlatformTarget.h"

#if MFX_PLATFORM_WINDOWS
#include "Platform/windows/Shell/Win32AppShell.h"
#elif MFX_PLATFORM_MACOS || MFX_PLATFORM_LINUX
#include "Platform/PlatformShellServicesFactory.h"

#if defined(MFX_ENTRY_RUNTIME_SCAFFOLD)
#include "Platform/posix/Shell/PosixScaffoldAppShell.h"
#elif defined(MFX_ENTRY_RUNTIME_POSIX_CORE)
#include "Platform/posix/Shell/PosixCoreAppShell.h"
#else
#include "MouseFx/Core/Shell/AppShellCore.h"
#endif
#endif

namespace mousefx::platform {

namespace {

#if MFX_PLATFORM_WINDOWS
class Win32PlatformAppShell final : public IPlatformAppShell {
public:
    explicit Win32PlatformAppShell(ShellPlatformServices services)
        : shell_(std::move(services)) {
    }

    bool Initialize(const AppShellStartOptions& options) override {
        return shell_.Initialize(options);
    }

    int RunMessageLoop() override {
        return shell_.RunMessageLoop();
    }

    void Shutdown() override {
        shell_.Shutdown();
    }

private:
    Win32AppShell shell_;
};
#elif MFX_PLATFORM_MACOS || MFX_PLATFORM_LINUX

void MergeDefaultPlatformServices(ShellPlatformServices* services) {
    if (!services) {
        return;
    }
    if (services->trayService && services->settingsLauncher && services->singleInstanceGuard &&
        services->eventLoopService) {
        return;
    }

    ShellPlatformServices defaults = CreateShellPlatformServices();
    if (!services->trayService) {
        services->trayService = std::move(defaults.trayService);
    }
    if (!services->settingsLauncher) {
        services->settingsLauncher = std::move(defaults.settingsLauncher);
    }
    if (!services->singleInstanceGuard) {
        services->singleInstanceGuard = std::move(defaults.singleInstanceGuard);
    }
    if (!services->dpiAwarenessService) {
        services->dpiAwarenessService = std::move(defaults.dpiAwarenessService);
    }
    if (!services->eventLoopService) {
        services->eventLoopService = std::move(defaults.eventLoopService);
    }
    if (!services->notifier) {
        services->notifier = std::move(defaults.notifier);
    }
}

#if !defined(MFX_ENTRY_RUNTIME_SCAFFOLD) && !defined(MFX_ENTRY_RUNTIME_POSIX_CORE)
class PosixPlatformAppShell final : public IPlatformAppShell {
public:
    explicit PosixPlatformAppShell(ShellPlatformServices services)
        : core_(std::make_unique<AppShellCore>(std::move(services))) {
    }

    bool Initialize(const AppShellStartOptions& options) override {
        if (!core_) {
            return false;
        }
        return core_->Initialize(options);
    }

    int RunMessageLoop() override {
        if (!core_) {
            return -1;
        }
        return core_->RunMessageLoop();
    }

    void Shutdown() override {
        if (core_) {
            core_->Shutdown();
        }
    }

private:
    std::unique_ptr<AppShellCore> core_{};
};
#endif

#else
class NullPlatformAppShell final : public IPlatformAppShell {
public:
    bool Initialize(const AppShellStartOptions& options) override {
        (void)options;
        return false;
    }

    int RunMessageLoop() override {
        return -1;
    }

    void Shutdown() override {
    }
};
#endif

} // namespace

std::unique_ptr<IPlatformAppShell> CreatePlatformAppShell(ShellPlatformServices services) {
#if MFX_PLATFORM_WINDOWS
    return std::make_unique<Win32PlatformAppShell>(std::move(services));
#elif MFX_PLATFORM_MACOS || MFX_PLATFORM_LINUX
    MergeDefaultPlatformServices(&services);

#if defined(MFX_ENTRY_RUNTIME_SCAFFOLD)
    return std::make_unique<PosixScaffoldAppShell>(std::move(services));
#elif defined(MFX_ENTRY_RUNTIME_POSIX_CORE)
    return std::make_unique<PosixCoreAppShell>(std::move(services));
#else
    return std::make_unique<PosixPlatformAppShell>(std::move(services));
#endif
#else
    (void)services;
    return std::make_unique<NullPlatformAppShell>();
#endif
}

} // namespace mousefx::platform
