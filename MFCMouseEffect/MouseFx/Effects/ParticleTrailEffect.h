#pragma once

#include "MouseFx/Core/Config/EffectConfig.h"
#include "MouseFx/Core/Effects/TrailEffectCompute.h"
#include "MouseFx/Interfaces/IMouseEffect.h"
#include "MouseFx/Interfaces/IParticleTrailEffectFallback.h"

#include <memory>
#include <string>

namespace mousefx {

class ParticleTrailOverlayLayer;

class ParticleTrailEffect final : public IMouseEffect {
public:
    explicit ParticleTrailEffect(const EffectConfig& config);
    ~ParticleTrailEffect() override;

    EffectCategory Category() const override { return EffectCategory::Trail; }
    const char* TypeName() const override { return "particle"; }

    bool Initialize() override;
    void Shutdown() override;

    void OnMouseMove(const ScreenPoint& pt) override;

private:
    std::unique_ptr<IParticleTrailEffectFallback> fallback_;
    ParticleTrailOverlayLayer* hostLayer_ = nullptr;
    std::string type_ = "particle";
    TrailEffectProfile computeProfile_{};
    TrailEffectThrottleProfile throttleProfile_{};
    ScreenPoint lastPoint_{};
    bool hasLastPoint_ = false;
    uint64_t lastEmitTickMs_ = 0;
    bool isChromatic_ = false;
};

} // namespace mousefx
