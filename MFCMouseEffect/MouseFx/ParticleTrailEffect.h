#pragma once

#include "IMouseEffect.h"
#include "ParticleTrailWindow.h"
#include <memory>

namespace mousefx {

class ParticleTrailEffect final : public IMouseEffect {
public:
    ParticleTrailEffect() = default;
    ~ParticleTrailEffect() override;

    EffectCategory Category() const override { return EffectCategory::Trail; }
    const char* TypeName() const override { return "particle"; }

    bool Initialize() override;
    void Shutdown() override;

    void OnMouseMove(const POINT& pt) override;

private:
    std::unique_ptr<ParticleTrailWindow> window_;
};

} // namespace mousefx
