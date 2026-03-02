#pragma once

#include "MouseFx/Core/Shell/AppShellStartOptions.h"

namespace mousefx::platform {

// Minimal app-shell lifecycle used by the process entrypoint.
class IPlatformAppShell {
public:
    virtual ~IPlatformAppShell() = default;

    virtual bool Initialize(const AppShellStartOptions& options) = 0;
    virtual int RunMessageLoop() = 0;
    virtual void Shutdown() = 0;
};

} // namespace mousefx::platform
