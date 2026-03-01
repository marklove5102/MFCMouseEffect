#include "pch.h"

#include "Platform/macos/Effects/MacosHoldPulseOverlayStyle.h"
#include "MouseFx/Core/Effects/HoldEffectCompute.h"
#include "MouseFx/Effects/HoldRouteCatalog.h"

namespace mousefx::macos_hold_pulse::detail {

#if defined(__APPLE__)
std::string NormalizeHoldType(const std::string& effectType) {
    return NormalizeHoldEffectType(effectType);
}

bool ContainsHoldToken(const std::string& holdType, const char* token) {
    return holdType.find(token) != std::string::npos;
}

HoldStyle ResolveHoldStyle(const std::string& holdType) {
    if (ContainsHoldToken(holdType, "hold_quantum_halo_gpu_v2") ||
        ContainsHoldToken(holdType, "hold_neon3d_gpu_v2") ||
        ContainsHoldToken(holdType, "quantum_halo")) {
        return HoldStyle::QuantumHalo;
    }
    if (ContainsHoldToken(holdType, mousefx::hold_route::kTypeFluxFieldGpuV2) ||
        ContainsHoldToken(holdType, mousefx::hold_route::kTypeFluxFieldCpu) ||
        ContainsHoldToken(holdType, "fluxfield") ||
        ContainsHoldToken(holdType, "flux_field")) {
        return HoldStyle::FluxField;
    }
    if (ContainsHoldToken(holdType, "scifi3d")) {
        return HoldStyle::Hologram;
    }
    if (holdType.find("lightning") != std::string::npos) {
        return HoldStyle::Lightning;
    }
    if (holdType.find("hex") != std::string::npos) {
        return HoldStyle::Hex;
    }
    if (holdType.find("hologram") != std::string::npos) {
        return HoldStyle::Hologram;
    }
    if (holdType.find("tech") != std::string::npos) {
        return HoldStyle::TechRing;
    }
    if (holdType.find("neon") != std::string::npos) {
        return HoldStyle::Neon;
    }
    return HoldStyle::Charge;
}
#endif

} // namespace mousefx::macos_hold_pulse::detail
