#pragma once

#include <memory>

namespace mousefx {
class ITextEffectFallback;
class ITrailEffectFallback;
class IParticleTrailEffectFallback;
}

namespace mousefx::platform {

std::unique_ptr<ITextEffectFallback> CreateTextEffectFallback();
std::unique_ptr<ITrailEffectFallback> CreateTrailEffectFallback();
std::unique_ptr<IParticleTrailEffectFallback> CreateParticleTrailEffectFallback();

} // namespace mousefx::platform
