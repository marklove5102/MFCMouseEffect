#pragma once

#include "MouseFx/Core/Effects/HoverEffectCompute.h"
#include "MouseFx/Interfaces/IMouseEffect.h"
#include "MouseFx/Styles/RippleStyle.h"

#include <string>

namespace mousefx {

class MacosHoverPulseEffect final : public IMouseEffect {
public:
    MacosHoverPulseEffect(std::string effectType, std::string themeName, int sizeScalePercent);
    ~MacosHoverPulseEffect() override;

    EffectCategory Category() const override { return EffectCategory::Hover; }
    const char* TypeName() const override { return effectType_.c_str(); }

    bool Initialize() override;
    void Shutdown() override;
    void OnHoverStart(const ScreenPoint& pt) override;
    void OnHoverEnd() override;

private:
    std::string effectType_{};
    std::string themeName_{};
    int sizeScalePercent_ = 100;
    RippleStyle style_{};
    HoverEffectProfile computeProfile_{};
    bool isChromatic_ = false;
    bool initialized_ = false;
};

} // namespace mousefx
