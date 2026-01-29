#include "pch.h"
#include "HoverEffect.h"
#include "ThemeStyle.h"
#include "Renderers/Hover/CrosshairRenderer.h"

namespace mousefx {

HoverEffect::HoverEffect(const std::string& themeName) {
    style_ = GetThemePalette(themeName).hover;
    isChromatic_ = (ToLowerAscii(themeName) == "chromatic");
}

HoverEffect::~HoverEffect() {
    Shutdown();
}

bool HoverEffect::Initialize() {
    return pool_.Initialize(2);
}

void HoverEffect::Shutdown() {
    OnHoverEnd();
    pool_.Shutdown();
}

void HoverEffect::OnHoverStart(const POINT& pt) {
    if (currentGlow_) return;

    ClickEvent ev{};
    ev.pt = pt;
    ev.button = MouseButton::Left;

    RenderParams params;
    params.loop = true;
    params.intensity = 1.0f;

    params.intensity = 1.0f;

    RippleStyle finalStyle = style_;
    if (isChromatic_) {
        finalStyle = MakeRandomStyle(style_);
    }

    currentGlow_ = pool_.ShowContinuous(ev, finalStyle, std::make_unique<CrosshairRenderer>(), params);
}

void HoverEffect::OnHoverEnd() {
    if (currentGlow_) {
        currentGlow_->Stop();
        currentGlow_ = nullptr;
    }
}

} // namespace mousefx
