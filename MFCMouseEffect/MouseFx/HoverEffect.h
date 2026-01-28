#pragma once

#include "IMouseEffect.h"
#include "RippleWindowPool.h"
#include <string>

namespace mousefx {

// Hover effect: shows a pulsing glow when the mouse is idle.
class HoverEffect final : public IMouseEffect {
public:
    explicit HoverEffect(const std::string& themeName);
    ~HoverEffect() override;

    EffectCategory Category() const override { return EffectCategory::Hover; }
    const char* TypeName() const override { return "glow"; }

    bool Initialize() override;
    void Shutdown() override;
    
    void OnHoverStart(const POINT& pt) override;
    void OnHoverEnd() override;

private:
    RippleWindowPool pool_{};
    RippleWindow* currentGlow_ = nullptr;
    RippleStyle style_{};
    bool isChromatic_ = false;
};

} // namespace mousefx
