#pragma once

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererScene.h"
#include "Platform/windows/Pet/Win32MouseCompanionRealRendererSceneRuntime.h"

namespace mousefx::windows {

Win32MouseCompanionRealRendererScene BuildWin32MouseCompanionRealRendererScene(
    const Win32MouseCompanionRealRendererSceneRuntime& runtime,
    int width,
    int height);

} // namespace mousefx::windows
