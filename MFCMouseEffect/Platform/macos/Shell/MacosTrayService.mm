#include "pch.h"

#include "Platform/macos/Shell/MacosTrayService.h"

#include "MouseFx/Core/Shell/IAppShellHost.h"
#include "Platform/macos/Shell/MacosTrayMenuFactory.h"
#include "Platform/macos/Shell/MacosTrayMenuLocalization.h"
#include "Platform/macos/Shell/MacosTrayRuntimeHelpers.h"

namespace mousefx {

#if defined(__APPLE__)

struct MacosTrayService::Impl {
    IAppShellHost* host = nullptr;
    macos_tray::MacosTrayMenuObjects tray{};
    bool started = false;
};

MacosTrayService::MacosTrayService()
    : impl_(std::make_unique<Impl>()) {}

MacosTrayService::~MacosTrayService() {
    Stop();
}

bool MacosTrayService::Start(IAppShellHost* host, bool showTrayIcon) {
    if (!impl_ || host == nullptr) {
        return false;
    }
    if (impl_->started) {
        return true;
    }

    impl_->host = host;
    if (!showTrayIcon) {
        impl_->started = true;
        return true;
    }

    __block bool started = true;
    macos_tray::RunOnMainThreadSync(^{
      const MacosTrayMenuText menuText = ResolveMacosTrayMenuText();
      if (!macos_tray::BuildMacosTrayMenu(host, menuText, &impl_->tray)) {
          started = false;
          return;
      }
      macos_tray::ScheduleAutoTriggerSettingsAction(impl_->tray.menu);
    });

    if (!started) {
        Stop();
        return false;
    }

    impl_->started = true;
    return true;
}

void MacosTrayService::Stop() {
    if (!impl_ || !impl_->started) {
        return;
    }

    macos_tray::RunOnMainThreadSync(^{
      macos_tray::ReleaseMacosTrayMenu(&impl_->tray);
    });

    impl_->host = nullptr;
    impl_->started = false;
}

void MacosTrayService::RequestExit() {
    // AppShellCore controls run-loop shutdown; tray exit action routes through host callback.
}

#else

struct MacosTrayService::Impl {};

MacosTrayService::MacosTrayService()
    : impl_(std::make_unique<Impl>()) {}

MacosTrayService::~MacosTrayService() = default;

bool MacosTrayService::Start(IAppShellHost* host, bool showTrayIcon) {
    (void)showTrayIcon;
    return host != nullptr;
}

void MacosTrayService::Stop() {
}

void MacosTrayService::RequestExit() {
}

#endif

} // namespace mousefx
