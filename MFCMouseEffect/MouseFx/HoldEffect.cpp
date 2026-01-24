#include "pch.h"
#include "HoldEffect.h"
#include "ThemeStyle.h"

namespace mousefx {

HoldEffect::HoldEffect(const std::string& themeName) {
    style_ = GetThemePalette(themeName).hold;
}

HoldEffect::~HoldEffect() {
    Shutdown();
}

bool HoldEffect::Initialize() {
    // Small pool for hold indicators
    return pool_.Initialize(5);
}

void HoldEffect::Shutdown() {
    pool_.Shutdown();
}

void HoldEffect::OnHoldStart(const POINT& pt, int button) {
    if (holdButton_ != 0) return; // Already holding?

    holdPoint_ = pt;
    holdButton_ = button;
    
    ClickEvent ev{};
    ev.pt = pt;
    ev.button = MouseButton::Left;

    RippleWindow::RenderParams params;
    params.loop = false;
    params.intensity = 1.0f;

    currentRipple_ = pool_.ShowContinuous(ev, style_, RippleWindow::DrawMode::ChargeRing, params);
}

void HoldEffect::OnHoldUpdate(const POINT& pt, DWORD durationMs) {
    holdPoint_ = pt;
    if (currentRipple_) {
        currentRipple_->UpdatePosition(pt);
    }
    (void)durationMs;
}

void HoldEffect::OnHoldEnd() {
    if (currentRipple_) {
        currentRipple_->Stop();
        currentRipple_ = nullptr;
    }
    holdButton_ = 0;
}

} // namespace mousefx
