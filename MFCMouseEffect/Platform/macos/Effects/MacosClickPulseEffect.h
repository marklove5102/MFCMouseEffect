#pragma once

#include "MouseFx/Core/Config/EffectConfig.h"
#include "MouseFx/Core/Effects/ClickEffectCompute.h"
#include "MouseFx/Interfaces/IMouseEffect.h"
#include "Platform/macos/Effects/MacosEffectRenderProfile.h"

#include <string>

namespace mousefx {

class MacosClickPulseEffect final : public IMouseEffect {
public:
    MacosClickPulseEffect(
        std::string effectType,
        std::string themeName,
        macos_effect_profile::ClickRenderProfile renderProfile,
        TextConfig textConfig);
    ~MacosClickPulseEffect() override;

    EffectCategory Category() const override { return EffectCategory::Click; }
    const char* TypeName() const override { return effectType_.c_str(); }

    bool Initialize() override;
    void Shutdown() override;
    void OnClick(const ClickEvent& event) override;

private:
    ClickEffectRenderCommand BuildTextClickCommand(
        const ClickEvent& event,
        ClickEffectRenderCommand command) const;

    std::string effectType_{};
    std::string themeName_{};
    macos_effect_profile::ClickRenderProfile renderProfile_{};
    TextConfig textConfig_{};
    bool isChromatic_ = false;
    bool initialized_ = false;
};

} // namespace mousefx
