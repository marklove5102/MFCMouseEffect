#include "pch.h"

#include "Platform/posix/Shell/PosixCoreAppShell.h"

#if MFX_PLATFORM_MACOS || MFX_PLATFORM_LINUX

#include "Platform/posix/Shell/PosixCoreWebSettingsProbe.h"
#include "MouseFx/Core/Control/AppController.h"
#include "MouseFx/Server/WebSettingsServer.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <thread>
#include <utility>

namespace mousefx::platform {
namespace {

char ToLowerAscii(char c) {
    if (c >= 'A' && c <= 'Z') {
        return static_cast<char>(c - 'A' + 'a');
    }
    return c;
}

bool EqualsIgnoreCaseAscii(std::string_view lhs, std::string_view rhs) {
    if (lhs.size() != rhs.size()) {
        return false;
    }
    for (size_t i = 0; i < lhs.size(); ++i) {
        if (ToLowerAscii(lhs[i]) != ToLowerAscii(rhs[i])) {
            return false;
        }
    }
    return true;
}

bool IsExitCommandLine(const std::string& line) {
    if (EqualsIgnoreCaseAscii(line, "exit")) {
        return true;
    }
    const std::string_view text = line;
    return text.find("\"cmd\"") != std::string_view::npos &&
           text.find("\"exit\"") != std::string_view::npos;
}

std::string BuildInputCaptureDegradedWarning(const AppController::InputCaptureRuntimeStatus& status) {
    using Reason = AppController::InputCaptureFailureReason;
    switch (status.reason) {
    case Reason::PermissionDenied:
        return "Global input capture is degraded. Grant Accessibility and Input Monitoring permissions to recover.";
    case Reason::Unsupported:
        return "Global input capture is not supported on this platform. Running with available features only.";
    case Reason::StartFailed:
    default:
        return std::string("Global input capture failed to start. Running in degraded mode. Error code: ") +
               std::to_string(status.error) + ".";
    }
}

} // namespace

PosixCoreAppShell::PosixCoreAppShell(ShellPlatformServices services)
    : services_(std::move(services)) {
}

bool PosixCoreAppShell::Initialize(const AppShellStartOptions& options) {
    if (initialized_) {
        return true;
    }
    if (!services_.settingsLauncher || !services_.singleInstanceGuard || !services_.eventLoopService) {
        return false;
    }
    if (!services_.singleInstanceGuard->Acquire(options.singleInstanceKey)) {
        return false;
    }

    if (services_.dpiAwarenessService) {
        services_.dpiAwarenessService->EnableForScreenCoords();
    }

    backgroundMode_ = !options.showTrayIcon || !services_.trayService;
    if (!backgroundMode_ && services_.trayService) {
        if (!services_.trayService->Start(this, options.showTrayIcon)) {
            services_.singleInstanceGuard->Release();
            return false;
        }
    } else {
        StartStdinExitMonitor();
    }

    appController_ = std::make_unique<AppController>();
    if (!appController_ || !appController_->Start()) {
        if (services_.notifier) {
            services_.notifier->ShowWarning(
                "MFCMouseEffect",
                "Core runtime failed to start. Check permissions and logs.");
        }
        appController_.reset();
        if (services_.trayService) {
            services_.trayService->Stop();
        }
        services_.singleInstanceGuard->Release();
        return false;
    }

    if (services_.notifier) {
        auto* notifier = services_.notifier.get();
        appController_->SetInputCaptureStatusCallback(
            [notifier](const AppController::InputCaptureRuntimeStatus& status) {
                if (!notifier || status.active) {
                    return;
                }
                notifier->ShowWarning(
                    "MFCMouseEffect",
                    BuildInputCaptureDegradedWarning(status));
            });
    }

    const AppController::InputCaptureRuntimeStatus inputCaptureStatus = appController_->InputCaptureStatus();
    if (!inputCaptureStatus.active && services_.notifier) {
        services_.notifier->ShowWarning(
            "MFCMouseEffect",
            BuildInputCaptureDegradedWarning(inputCaptureStatus));
    }

    if (!SetupRegressionWebSettingsProbe() && services_.notifier) {
        services_.notifier->ShowWarning(
            "MFCMouseEffect",
            "Core WebSettings probe output failed. Check MFX_CORE_WEB_SETTINGS_PROBE_FILE.");
    }

    initialized_ = true;
    return true;
}

int PosixCoreAppShell::RunMessageLoop() {
    if (!services_.eventLoopService) {
        return -1;
    }
    return services_.eventLoopService->Run();
}

void PosixCoreAppShell::Shutdown() {
    if (!initialized_) {
        return;
    }
    if (webSettings_) {
        webSettings_->Stop();
        webSettings_.reset();
    }
    if (appController_) {
        appController_->SetInputCaptureStatusCallback(nullptr);
        appController_->Stop();
        appController_.reset();
    }
    if (services_.trayService) {
        services_.trayService->Stop();
    }
    if (services_.singleInstanceGuard) {
        services_.singleInstanceGuard->Release();
    }
    initialized_ = false;
}

AppController* PosixCoreAppShell::AppControllerForShell() noexcept {
    return appController_.get();
}

void PosixCoreAppShell::OpenSettingsFromShell() {
    if (!PostShellTask([this]() {
            ShowWebSettings();
        })) {
        ShowWebSettings();
    }
}

void PosixCoreAppShell::RequestExitFromShell() {
    if (!PostShellTask([this]() {
            RequestExitOnLoop();
        })) {
        RequestExitOnLoop();
    }
}

bool PosixCoreAppShell::PostShellTask(std::function<void()> task) {
    if (!initialized_ || !services_.eventLoopService || !task) {
        return false;
    }
    return services_.eventLoopService->PostTask(std::move(task));
}

void PosixCoreAppShell::RequestExitOnLoop() {
    if (services_.trayService && !backgroundMode_) {
        services_.trayService->RequestExit();
    }
    if (services_.eventLoopService) {
        services_.eventLoopService->RequestExit();
    }
}

void PosixCoreAppShell::ShowWebSettings() {
    if (backgroundMode_ || !appController_ || !services_.settingsLauncher) {
        return;
    }

    if (!webSettings_) {
        webSettings_ = std::make_unique<WebSettingsServer>(appController_.get());
    }
    if (!webSettings_->IsRunning()) {
        webSettings_->RotateToken();
        if (!webSettings_->Start()) {
            if (services_.notifier) {
                services_.notifier->ShowWarning("MFCMouseEffect", "Web settings server start failed.");
            }
            return;
        }
    }

    if (!services_.settingsLauncher->OpenUrlUtf8(webSettings_->Url()) && services_.notifier) {
        services_.notifier->ShowWarning("MFCMouseEffect", "Open core settings URL failed.");
    }
}

bool PosixCoreAppShell::SetupRegressionWebSettingsProbe() {
    const std::string probeFilePath = ReadCoreWebSettingsProbeFilePath();
    const std::string launchProbeFilePath = ReadCoreWebSettingsLaunchProbeFilePath();
    if (!appController_) {
        return true;
    }
    if (probeFilePath.empty() && launchProbeFilePath.empty()) {
        return true;
    }

    if (!webSettings_) {
        webSettings_ = std::make_unique<WebSettingsServer>(appController_.get());
    }
    if (!webSettings_->IsRunning()) {
        webSettings_->RotateToken();
        if (!webSettings_->Start()) {
            return false;
        }
    }

    bool ok = true;
    if (!probeFilePath.empty()) {
        ok = WriteCoreWebSettingsProbeFile(probeFilePath, *webSettings_) && ok;
    }

    if (!launchProbeFilePath.empty()) {
        const std::string settingsUrl = webSettings_->Url();
        const bool opened = services_.settingsLauncher && services_.settingsLauncher->OpenUrlUtf8(settingsUrl);
        ok = WriteCoreWebSettingsLaunchProbeFile(launchProbeFilePath, settingsUrl, opened) && ok;
        ok = opened && ok;
    }

    return ok;
}

void PosixCoreAppShell::StartStdinExitMonitor() {
    if (stdinMonitorStarted_) {
        return;
    }
    stdinMonitorStarted_ = true;

    auto* eventLoop = services_.eventLoopService.get();
    std::thread([eventLoop]() {
        std::string line;
        while (std::getline(std::cin, line)) {
            if (IsExitCommandLine(line) && eventLoop) {
                eventLoop->RequestExit();
                return;
            }
        }
        if (eventLoop) {
            eventLoop->RequestExit();
        }
    }).detach();
}

} // namespace mousefx::platform

#endif
