#include "pch.h"
#include "RippleEffect.h"
#include "ThemeStyle.h"
#include "StandardRenderers.h"

namespace mousefx {

RippleEffect::RippleEffect(const std::string& themeName) {
    style_ = GetThemePalette(themeName).click;
}

RippleEffect::~RippleEffect() {
    Shutdown();
}

bool RippleEffect::Initialize() {
    // Standard pool size.
    return pool_.Initialize(10);
}

void RippleEffect::Shutdown() {
    pool_.Shutdown();
}

void RippleEffect::OnClick(const ClickEvent& event) {
    RenderParams params;
    params.loop = false;
    params.intensity = 1.0f;
    pool_.ShowRipple(event, style_, std::make_unique<RippleRenderer>(), params);
}

} // namespace mousefx
