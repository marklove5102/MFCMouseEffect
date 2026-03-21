#pragma once

#include <mutex>

#include "Platform/windows/Pet/IWin32MouseCompanionRendererBackend.h"

namespace mousefx::windows {

class Win32MouseCompanionRealRendererBackend final : public IWin32MouseCompanionRendererBackend {
public:
    bool Start() override;
    void Shutdown() override;
    bool IsReady() const override;
    std::string LastErrorReason() const override;
    void Render(
        const Win32MouseCompanionRendererInput& input,
        Gdiplus::Graphics* graphics,
        int width,
        int height) const override;
    Win32MouseCompanionRendererBackendRuntimeDiagnostics ReadRuntimeDiagnostics() const override;

private:
    bool ready_{false};
    std::string lastErrorReason_{"pending_implementation"};
    mutable std::mutex runtimeDiagnosticsMutex_{};
    mutable Win32MouseCompanionRendererBackendRuntimeDiagnostics runtimeDiagnostics_{};
};

void RegisterWin32MouseCompanionRealRendererBackend();

} // namespace mousefx::windows
