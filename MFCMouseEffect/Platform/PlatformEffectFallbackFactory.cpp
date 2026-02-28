#include "pch.h"

#include "Platform/PlatformEffectFallbackFactory.h"

#if defined(_WIN32)
#include "Platform/windows/Effects/Win32ParticleTrailEffectFallback.h"
#include "Platform/windows/Effects/Win32TextEffectFallback.h"
#include "Platform/windows/Effects/Win32TrailEffectFallback.h"
#elif defined(__APPLE__)
#include "Platform/macos/Effects/MacosTextEffectFallback.h"
#else
#include "MouseFx/Interfaces/NullParticleTrailEffectFallback.h"
#include "MouseFx/Interfaces/NullTextEffectFallback.h"
#include "MouseFx/Interfaces/NullTrailEffectFallback.h"
#endif

namespace mousefx::platform {

std::unique_ptr<ITextEffectFallback> CreateTextEffectFallback() {
#if defined(_WIN32)
    return std::make_unique<Win32TextEffectFallback>();
#elif defined(__APPLE__)
    return std::make_unique<MacosTextEffectFallback>();
#else
    return std::make_unique<NullTextEffectFallback>();
#endif
}

std::unique_ptr<ITrailEffectFallback> CreateTrailEffectFallback() {
#if defined(_WIN32)
    return std::make_unique<Win32TrailEffectFallback>();
#elif defined(__APPLE__)
    return {};
#else
    return std::make_unique<NullTrailEffectFallback>();
#endif
}

std::unique_ptr<IParticleTrailEffectFallback> CreateParticleTrailEffectFallback() {
#if defined(_WIN32)
    return std::make_unique<Win32ParticleTrailEffectFallback>();
#elif defined(__APPLE__)
    return {};
#else
    return std::make_unique<NullParticleTrailEffectFallback>();
#endif
}

} // namespace mousefx::platform
