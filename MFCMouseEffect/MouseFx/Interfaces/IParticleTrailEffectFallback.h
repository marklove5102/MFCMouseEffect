#pragma once

#include "MouseFx/Core/Effects/TrailEffectCompute.h"

namespace mousefx {

class IParticleTrailEffectFallback {
public:
    virtual ~IParticleTrailEffectFallback() = default;

    virtual bool Create() = 0;
    virtual void Shutdown() = 0;
    virtual void SetChromatic(bool chromatic) = 0;
    virtual void AddCommand(const TrailEffectRenderCommand& command) = 0;
    virtual void Clear() = 0;
};

} // namespace mousefx
