#pragma once

#include <string>

namespace mousefx::windows {

struct Win32MouseCompanionRealRendererSceneRuntime;

struct Win32MouseCompanionRealRendererModelNodeAdapterProfile final {
    float influence{0.0f};
    float centerOffsetX{0.0f};
    float centerOffsetY{0.0f};
    float faceOffsetX{0.0f};
    float faceOffsetY{0.0f};
    float overlayOffsetX{0.0f};
    float overlayOffsetY{0.0f};
    float adornmentOffsetX{0.0f};
    float adornmentOffsetY{0.0f};
    float groundingOffsetX{0.0f};
    float groundingOffsetY{0.0f};
    float whiskerBias{0.0f};
    float blushLift{0.0f};
    std::string brief{"preview_only/0.00"};
};

Win32MouseCompanionRealRendererModelNodeAdapterProfile
BuildWin32MouseCompanionRealRendererModelNodeAdapterProfile(
    const Win32MouseCompanionRealRendererSceneRuntime& runtime);

} // namespace mousefx::windows
