#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionRendererBackendFactory.h"

#include "Platform/windows/Pet/Win32MouseCompanionRendererBackendRegistry.h"
#include "Platform/windows/Pet/Win32MouseCompanionPlaceholderRenderer.h"

namespace mousefx::windows {

std::unique_ptr<IWin32MouseCompanionRendererBackend> CreateDefaultWin32MouseCompanionRendererBackend() {
    RegisterWin32MouseCompanionPlaceholderRendererBackend();
    auto backend = Win32MouseCompanionRendererBackendRegistry::Instance().CreateHighestPriority();
    if (backend) {
        return backend;
    }
    return std::make_unique<Win32MouseCompanionPlaceholderRenderer>();
}

} // namespace mousefx::windows
