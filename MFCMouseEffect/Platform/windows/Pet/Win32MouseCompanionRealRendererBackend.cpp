#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererBackend.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererCapabilities.h"
#include "Platform/windows/Pet/Win32MouseCompanionRendererBackendRegistry.h"

namespace mousefx::windows {

bool Win32MouseCompanionRealRendererBackend::Start() {
    lastErrorReason_ = DescribeWin32MouseCompanionRealRendererStartFailure();
    return false;
}

void Win32MouseCompanionRealRendererBackend::Shutdown() {}

bool Win32MouseCompanionRealRendererBackend::IsReady() const {
    return false;
}

std::string Win32MouseCompanionRealRendererBackend::LastErrorReason() const {
    return lastErrorReason_;
}

void Win32MouseCompanionRealRendererBackend::Render(
    const Win32MouseCompanionRendererInput&,
    Gdiplus::Graphics*,
    int,
    int) const {}

void RegisterWin32MouseCompanionRealRendererBackend() {
    static Win32MouseCompanionRendererBackendRegistrar<Win32MouseCompanionRealRendererBackend> registrar(
        "real",
        200,
        &EvaluateWin32MouseCompanionRealRendererAvailability);
    (void)registrar;
}

} // namespace mousefx::windows
