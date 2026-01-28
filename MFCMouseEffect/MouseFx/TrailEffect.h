#pragma once

#include "IMouseEffect.h"
#include "TrailWindow.h"
#include <memory>

namespace mousefx {

class TrailEffect final : public IMouseEffect {
public:
    explicit TrailEffect(const std::string& themeName);
    ~TrailEffect() override;

    EffectCategory Category() const override { return EffectCategory::Trail; }
    const char* TypeName() const override { return "line"; }

    bool Initialize() override;
    void Shutdown() override;
    void OnMouseMove(const POINT& pt) override;

private:
    std::unique_ptr<TrailWindow> window_;
    bool isChromatic_ = false;
};

} // namespace mousefx
