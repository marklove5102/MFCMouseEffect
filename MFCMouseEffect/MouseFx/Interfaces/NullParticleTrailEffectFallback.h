#pragma once

#include "MouseFx/Interfaces/IParticleTrailEffectFallback.h"

namespace mousefx {

class NullParticleTrailEffectFallback final : public IParticleTrailEffectFallback {
public:
    bool Create() override { return false; }
    void Shutdown() override {}
    void SetChromatic(bool) override {}
    void AddCommand(const TrailEffectRenderCommand&) override {}
    void Clear() override {}
};

} // namespace mousefx
