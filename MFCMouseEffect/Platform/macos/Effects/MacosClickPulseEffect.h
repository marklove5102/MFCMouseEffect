#pragma once

#include "MouseFx/Interfaces/IMouseEffect.h"

#include <string>

namespace mousefx {

class MacosClickPulseEffect final : public IMouseEffect {
public:
    explicit MacosClickPulseEffect(std::string themeName);
    ~MacosClickPulseEffect() override;

    EffectCategory Category() const override { return EffectCategory::Click; }
    const char* TypeName() const override { return "click_pulse"; }

    bool Initialize() override;
    void Shutdown() override;
    void OnClick(const ClickEvent& event) override;

private:
    std::string themeName_{};
    bool initialized_ = false;
};

} // namespace mousefx
