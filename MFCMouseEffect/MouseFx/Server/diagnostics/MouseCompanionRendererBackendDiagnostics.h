#pragma once

#include <string>

#include "MouseFx/Core/Control/AppController.h"

namespace mousefx {

struct ConfiguredMouseCompanionRendererBackendPreferenceDiagnostics {
    bool effective{false};
    std::string status;
};

ConfiguredMouseCompanionRendererBackendPreferenceDiagnostics
EvaluateConfiguredMouseCompanionRendererBackendPreferenceDiagnostics(
    const AppController::MouseCompanionRuntimeStatus& status);

} // namespace mousefx
