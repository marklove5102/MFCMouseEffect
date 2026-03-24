#pragma once

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererScene.h"
#include "Platform/windows/Pet/Win32MouseCompanionRealRendererSceneRuntime.h"

namespace mousefx::windows {

void BuildWin32MouseCompanionRealRendererModelSceneGraph(
    const Win32MouseCompanionRealRendererSceneRuntime& runtime,
    Win32MouseCompanionRealRendererScene& scene);

} // namespace mousefx::windows
