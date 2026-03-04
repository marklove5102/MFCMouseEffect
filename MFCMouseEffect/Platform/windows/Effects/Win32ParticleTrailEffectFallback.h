#pragma once

#include "MouseFx/Interfaces/IParticleTrailEffectFallback.h"
#include "Platform/windows/Effects/ParticleTrailWindow.h"

namespace mousefx {

class Win32ParticleTrailEffectFallback final : public IParticleTrailEffectFallback {
public:
    bool Create() override;
    void Shutdown() override;
    void SetChromatic(bool chromatic) override;
    void AddCommand(const TrailEffectRenderCommand& command) override;
    void Clear() override;

private:
    ParticleTrailWindow window_{};
    bool created_ = false;
};

} // namespace mousefx
