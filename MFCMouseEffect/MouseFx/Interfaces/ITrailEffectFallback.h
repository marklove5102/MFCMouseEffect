#pragma once

#include <memory>

#include "MouseFx/Interfaces/ITrailRenderer.h"

namespace mousefx {

class ITrailEffectFallback {
public:
    virtual ~ITrailEffectFallback() = default;

    virtual bool Create() = 0;
    virtual void Shutdown() = 0;
    virtual void Configure(bool isChromatic, int durationMs, int maxPoints, std::unique_ptr<ITrailRenderer> renderer) = 0;
    virtual void AddPoint(const TrailPoint& point) = 0;
};

} // namespace mousefx
