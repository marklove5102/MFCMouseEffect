#include "pch.h"

#include "Platform/macos/Effects/MacosHoverPulseOverlayStyle.h"
#include "MouseFx/Core/Effects/HoverEffectCompute.h"

namespace mousefx::macos_hover_pulse {

#if defined(__APPLE__)
std::string NormalizeHoverType(const std::string& effectType) {
    return NormalizeHoverEffectType(effectType);
}

#endif

} // namespace mousefx::macos_hover_pulse
