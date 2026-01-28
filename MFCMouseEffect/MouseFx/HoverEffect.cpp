#include "pch.h"
#include "HoverEffect.h"
#include "ThemeStyle.h"
#include "StandardRenderers.h"

namespace mousefx {

HoverEffect::HoverEffect(const std::string& themeName) {
    style_ = GetThemePalette(themeName).hover;
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

    currentGlow_ = pool_.ShowContinuous(ev, style_, std::make_unique<CrosshairRenderer>(), params);
}

void HoverEffect::OnHoverEnd() {
    if (currentGlow_) {
        currentGlow_->Stop();
        currentGlow_ = nullptr;
    }
}

} // namespace mousefx
