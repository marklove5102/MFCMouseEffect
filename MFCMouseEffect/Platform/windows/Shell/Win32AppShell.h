#pragma once

#include <memory>

#include "MouseFx/Core/Shell/AppShellStartOptions.h"
#include "MouseFx/Core/Shell/ShellPlatformServices.h"

namespace mousefx {

class AppShellCore;

// Windows entry shell that wires platform services into AppShellCore.
class Win32AppShell final {
public:
    explicit Win32AppShell(ShellPlatformServices services = {});
    ~Win32AppShell();

    Win32AppShell(const Win32AppShell&) = delete;
    Win32AppShell& operator=(const Win32AppShell&) = delete;

    bool Initialize(const AppShellStartOptions& options);
    int RunMessageLoop();
    void Shutdown();

private:
    std::unique_ptr<AppShellCore> core_{};
};

} // namespace mousefx
