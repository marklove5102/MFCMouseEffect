#pragma once

#include "MouseFx/Interfaces/IMouseEffect.h"

#include <string>

namespace mousefx {

class MacosScrollPulseEffect final : public IMouseEffect {
public:
    explicit MacosScrollPulseEffect(std::string themeName);
    ~MacosScrollPulseEffect() override;

    EffectCategory Category() const override { return EffectCategory::Scroll; }
    const char* TypeName() const override { return "scroll_pulse"; }

    bool Initialize() override;
    void Shutdown() override;
    void OnScroll(const ScrollEvent& event) override;

private:
    std::string themeName_{};
    bool initialized_ = false;
};

} // namespace mousefx
