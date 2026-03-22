#pragma once

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererMotionProfile.h"
#include "Platform/windows/Pet/Win32MouseCompanionRealRendererPaletteProfile.h"
#include "Platform/windows/Pet/Win32MouseCompanionRealRendererScene.h"
#include "Platform/windows/Pet/Win32MouseCompanionRealRendererSceneRuntime.h"
#include "Platform/windows/Pet/Win32MouseCompanionRealRendererStyleProfile.h"

namespace mousefx::windows {

void BuildWin32MouseCompanionRealRendererPalette(
    const Win32MouseCompanionRealRendererSceneRuntime& runtime,
    const Win32MouseCompanionRealRendererMotionProfile& profile,
    const Win32MouseCompanionRealRendererStyleProfile& style,
    const Win32MouseCompanionRealRendererPaletteProfile& palette,
    Win32MouseCompanionRealRendererScene& scene);

} // namespace mousefx::windows
