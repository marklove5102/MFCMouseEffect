#include "pch.h"
#include "HoldRouteCatalog.h"
#include "MouseFx/Utils/StringUtils.h"

namespace mousefx::hold_route {

std::string NormalizeHoldEffectTypeAlias(const std::string& type) {
    const std::string lowered = ToLowerAscii(type);
    if (lowered == kTypeQuantumHaloGpuV2Legacy) {
        return kTypeQuantumHaloGpuV2;
    }
    if (lowered == kTypeHologramLegacy) {
        return kTypeHologram;
    }
    if (lowered == kTypeNeon3DLegacy) {
        return kTypeNeon3D;
    }
    return lowered;
}

bool IsGpuV2RouteType(const std::string& type) {
    return type.find("_gpu_v2") != std::string::npos;
}

bool IsQuantumHaloGpuV2Type(const std::string& type) {
    const std::string normalized = NormalizeHoldEffectTypeAlias(type);
    return normalized == kTypeQuantumHaloGpuV2;
}

bool IsFluxFieldGpuV2Type(const std::string& type) {
    const std::string normalized = NormalizeHoldEffectTypeAlias(type);
    return normalized == kTypeFluxFieldGpuV2;
}

bool IsQuantumHaloGpuV2DirectType(const std::string& type) {
    return IsQuantumHaloGpuV2Type(type);
}

const char* RouteReasonForType(const std::string& type) {
    const std::string normalized = NormalizeHoldEffectTypeAlias(type);
    if (normalized == kTypeQuantumHaloGpuV2) {
        return kRouteReasonQuantumHaloGpuV2;
    }
    if (normalized == kTypeFluxFieldGpuV2) {
        return kRouteReasonFluxFieldGpuV2;
    }
    return "";
}

} // namespace mousefx::hold_route
