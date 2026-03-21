#pragma once

#include <string>

#include "MouseFx/Core/Control/AppController.h"

namespace mousefx {

struct ConfiguredMouseCompanionRendererBackendPreferenceDiagnostics {
    bool effective{false};
    std::string status;
};

struct MouseCompanionRealRendererPreviewDiagnostics {
    bool rolloutEnabled{false};
    bool previewSelected{false};
    bool previewActive{false};
    bool renderedFrame{false};
    uint64_t renderedFrameCount{0};
    uint64_t lastRenderTickMs{0};
    std::string availabilityReason;
    bool modelReady{false};
    bool actionLibraryReady{false};
    bool appearanceProfileReady{false};
    bool poseFrameAvailable{false};
    bool poseBindingConfigured{false};
    int surfaceWidth{0};
    int surfaceHeight{0};
    std::string actionName{"idle"};
    std::string reactiveActionName{"idle"};
    float actionIntensity{0.0f};
    float reactiveActionIntensity{0.0f};
    std::string modelSourceFormat{"unknown"};
};

ConfiguredMouseCompanionRendererBackendPreferenceDiagnostics
EvaluateConfiguredMouseCompanionRendererBackendPreferenceDiagnostics(
    const AppController::MouseCompanionRuntimeStatus& status);

MouseCompanionRealRendererPreviewDiagnostics
EvaluateMouseCompanionRealRendererPreviewDiagnostics(
    const AppController::MouseCompanionRuntimeStatus& status);

} // namespace mousefx
