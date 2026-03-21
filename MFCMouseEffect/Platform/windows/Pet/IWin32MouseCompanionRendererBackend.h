#pragma once

#include <gdiplus.h>
#include <string>

#include "Platform/windows/Pet/Win32MouseCompanionRendererInput.h"

namespace mousefx::windows {

struct Win32MouseCompanionRendererBackendRuntimeDiagnostics {
    std::string backendName;
    bool ready{false};
    bool renderedFrame{false};
    uint64_t renderedFrameCount{0};
    uint64_t lastRenderTickMs{0};
    std::string actionName{"idle"};
    std::string reactiveActionName{"idle"};
    float actionIntensity{0.0f};
    float reactiveActionIntensity{0.0f};
    bool modelReady{false};
    bool actionLibraryReady{false};
    bool appearanceProfileReady{false};
    bool poseFrameAvailable{false};
    bool poseBindingConfigured{false};
    int facingDirection{1};
    int surfaceWidth{0};
    int surfaceHeight{0};
    std::string modelSourceFormat{"unknown"};
};

class IWin32MouseCompanionRendererBackend {
public:
    virtual ~IWin32MouseCompanionRendererBackend() = default;

    virtual bool Start() = 0;
    virtual void Shutdown() = 0;
    virtual bool IsReady() const = 0;
    virtual std::string LastErrorReason() const = 0;
    virtual void Render(
        const Win32MouseCompanionRendererInput& input,
        Gdiplus::Graphics* graphics,
        int width,
        int height) const = 0;
    virtual Win32MouseCompanionRendererBackendRuntimeDiagnostics ReadRuntimeDiagnostics() const = 0;
};

} // namespace mousefx::windows
