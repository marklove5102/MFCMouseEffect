#pragma once

#include <string>

namespace mousefx::macos_hold_pulse::detail {

#if defined(__APPLE__)
enum class HoldStyle {
    Charge,
    Lightning,
    Hex,
    TechRing,
    Hologram,
    Neon,
    QuantumHalo,
    FluxField,
};

std::string NormalizeHoldType(const std::string& effectType);
HoldStyle ResolveHoldStyle(const std::string& holdType);
#endif

} // namespace mousefx::macos_hold_pulse::detail
