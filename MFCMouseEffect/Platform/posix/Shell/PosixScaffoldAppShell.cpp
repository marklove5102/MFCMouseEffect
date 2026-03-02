#include "pch.h"

#include "Platform/posix/Shell/PosixScaffoldAppShell.h"

#if MFX_PLATFORM_MACOS || MFX_PLATFORM_LINUX

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

} // namespace

PosixScaffoldAppShell::PosixScaffoldAppShell(ShellPlatformServices services)
    : services_(std::move(services)) {
}

bool PosixScaffoldAppShell::Initialize(const AppShellStartOptions& options) {
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
    scaffoldRuntime_.SetRuntimeMode(static_cast<bool>(services_.trayService), backgroundMode_);
    scaffoldRuntime_.Start([this](const std::string& title, const std::string& message) {
        if (services_.notifier) {
            services_.notifier->ShowWarning(title, message);
        }
    });

    if (!backgroundMode_ && services_.trayService) {
        if (!services_.trayService->Start(this, options.showTrayIcon)) {
            scaffoldRuntime_.Stop();
            services_.singleInstanceGuard->Release();
            return false;
        }
    } else {
        StartStdinExitMonitor();
    }

    initialized_ = true;
    return true;
}

int PosixScaffoldAppShell::RunMessageLoop() {
    if (!services_.eventLoopService) {
        return -1;
    }
    return services_.eventLoopService->Run();
}

void PosixScaffoldAppShell::Shutdown() {
    if (!initialized_) {
        return;
    }
    if (services_.trayService) {
        services_.trayService->Stop();
    }
    if (services_.singleInstanceGuard) {
        services_.singleInstanceGuard->Release();
    }
    scaffoldRuntime_.Stop();
    initialized_ = false;
}

AppController* PosixScaffoldAppShell::AppControllerForShell() noexcept {
    return nullptr;
}

void PosixScaffoldAppShell::OpenSettingsFromShell() {
    if (!services_.settingsLauncher) {
        return;
    }
    if (!services_.settingsLauncher->OpenUrlUtf8(scaffoldRuntime_.SettingsUrl()) && services_.notifier) {
        services_.notifier->ShowWarning("MFCMouseEffect", "Open scaffold settings URL failed.");
    }
}

void PosixScaffoldAppShell::RequestExitFromShell() {
    if (services_.trayService && !backgroundMode_) {
        services_.trayService->RequestExit();
    }
    if (services_.eventLoopService) {
        services_.eventLoopService->RequestExit();
    }
}

void PosixScaffoldAppShell::StartStdinExitMonitor() {
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
