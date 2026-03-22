#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererCapabilities.h"

#include "MouseFx/Utils/StringUtils.h"
#include "Platform/windows/Pet/Win32MouseCompanionRealRendererAssetResources.h"

#include <cstdlib>

namespace mousefx::windows {
namespace {

constexpr const char* kRealRendererEnableEnvVar = "MFX_WIN32_MOUSE_COMPANION_REAL_RENDERER_ENABLE";

std::string ReadEnvCopy(const char* key) {
    if (!key || !*key) {
        return {};
    }
    char* raw = nullptr;
    size_t rawSize = 0;
    const errno_t result = _dupenv_s(&raw, &rawSize, key);
    if (result != 0 || !raw || rawSize == 0) {
        if (raw) {
            free(raw);
        }
        return {};
    }
    std::string value(raw);
    free(raw);
    return value;
}

bool IsEnabledByEnv(const char* key) {
    const std::string raw = ReadEnvCopy(key);
    if (raw.empty()) {
        return false;
    }
    const std::string normalized = ToLowerAscii(TrimAscii(raw));
    return normalized == "1" || normalized == "true" || normalized == "yes" || normalized == "on";
}

} // namespace

bool IsWin32MouseCompanionRealRendererAssetResourceAdapterReady() {
    return true;
}

bool IsWin32MouseCompanionRealRendererSceneRuntimeAdapterReady() {
    return true;
}

bool IsWin32MouseCompanionRealRendererDrawExecutionReady() {
    return true;
}

bool IsWin32MouseCompanionRealRendererRolloutEnabled() {
    return IsEnabledByEnv(kRealRendererEnableEnvVar);
}

std::vector<std::string> ListWin32MouseCompanionRealRendererUnmetRequirements() {
    std::vector<std::string> unmetRequirements;
    if (!IsWin32MouseCompanionRealRendererAssetResourceAdapterReady()) {
        unmetRequirements.push_back("asset_resource_adapter");
    }
    if (!IsWin32MouseCompanionRealRendererSceneRuntimeAdapterReady()) {
        unmetRequirements.push_back("scene_runtime_adapter");
    }
    if (!IsWin32MouseCompanionRealRendererDrawExecutionReady()) {
        unmetRequirements.push_back("renderer_draw_execution");
    }
    return unmetRequirements;
}

Win32MouseCompanionRendererBackendRegistry::Availability
EvaluateWin32MouseCompanionRealRendererAvailability() {
    Win32MouseCompanionRendererBackendRegistry::Availability availability{};
    availability.unmetRequirements = ListWin32MouseCompanionRealRendererUnmetRequirements();
    if (!availability.unmetRequirements.empty()) {
        availability.available = false;
        availability.reason = "requirements_unmet";
        return availability;
    }
    if (!IsWin32MouseCompanionRealRendererRolloutEnabled()) {
        availability.available = false;
        availability.reason = "rollout_disabled";
        return availability;
    }
    availability.available = true;
    availability.reason.clear();
    return availability;
}

std::string DescribeWin32MouseCompanionRealRendererStartFailure() {
    if (!ListWin32MouseCompanionRealRendererUnmetRequirements().empty()) {
        return "requirements_unmet";
    }
    if (!IsWin32MouseCompanionRealRendererRolloutEnabled()) {
        return "rollout_disabled";
    }
    return {};
}

} // namespace mousefx::windows
