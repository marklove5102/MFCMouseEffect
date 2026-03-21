#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererCapabilities.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererAssetResources.h"

namespace mousefx::windows {

bool IsWin32MouseCompanionRealRendererAssetResourceAdapterReady() {
    return true;
}

std::vector<std::string> ListWin32MouseCompanionRealRendererUnmetRequirements() {
    std::vector<std::string> unmetRequirements;
    if (!IsWin32MouseCompanionRealRendererAssetResourceAdapterReady()) {
        unmetRequirements.push_back("asset_resource_adapter");
    }
    unmetRequirements.push_back("scene_runtime_adapter");
    unmetRequirements.push_back("renderer_draw_execution");
    return unmetRequirements;
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
