#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererCapabilities.h"

namespace mousefx::windows {

std::vector<std::string> ListWin32MouseCompanionRealRendererUnmetRequirements() {
    return {
        "asset_resource_adapter",
        "scene_runtime_adapter",
        "renderer_draw_execution",
    };
}

Win32MouseCompanionRendererBackendRegistry::Availability
EvaluateWin32MouseCompanionRealRendererAvailability() {
    Win32MouseCompanionRendererBackendRegistry::Availability availability{};
    availability.available = false;
    availability.reason = "requirements_unmet";
    availability.unmetRequirements = ListWin32MouseCompanionRealRendererUnmetRequirements();
    return availability;
}

std::string DescribeWin32MouseCompanionRealRendererStartFailure() {
    return "requirements_unmet";
}

} // namespace mousefx::windows
