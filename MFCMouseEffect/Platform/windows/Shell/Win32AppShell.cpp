#include "pch.h"

#include "Platform/windows/Shell/Win32AppShell.h"

#include <utility>

#include "MouseFx/Core/Shell/AppShellCore.h"
#include "Platform/PlatformShellServicesFactory.h"

namespace mousefx {

namespace {

void MergeDefaultPlatformServices(ShellPlatformServices* services) {
    if (!services) {
        return;
    }
    if (services->trayService && services->settingsLauncher && services->singleInstanceGuard &&
        services->eventLoopService) {
        return;
    }

    ShellPlatformServices defaults = platform::CreateShellPlatformServices();
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

} // namespace

Win32AppShell::Win32AppShell(ShellPlatformServices services) {
    MergeDefaultPlatformServices(&services);
    core_ = std::make_unique<AppShellCore>(std::move(services));
}

Win32AppShell::~Win32AppShell() {
    Shutdown();
}

bool Win32AppShell::Initialize(const AppShellStartOptions& options) {
    if (!core_) {
        return false;
    }
    return core_->Initialize(options);
}

int Win32AppShell::RunMessageLoop() {
    if (!core_) {
        return -1;
    }
    return core_->RunMessageLoop();
}

void Win32AppShell::Shutdown() {
    if (core_) {
        core_->Shutdown();
    }
}

} // namespace mousefx
