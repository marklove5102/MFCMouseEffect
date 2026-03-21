#pragma once

#include "Platform/windows/Pet/Win32MouseCompanionPlaceholderScene.h"
#include "Platform/windows/Pet/Win32MouseCompanionRendererRuntime.h"

namespace mousefx::windows {

Win32MouseCompanionPlaceholderScene BuildWin32MouseCompanionPlaceholderScene(
    const Win32MouseCompanionRendererRuntime& runtime,
    int width,
    int height);

} // namespace mousefx::windows
