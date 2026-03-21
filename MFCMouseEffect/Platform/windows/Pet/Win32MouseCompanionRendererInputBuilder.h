#pragma once

#include "Platform/windows/Pet/Win32MouseCompanionRendererInput.h"
#include "Platform/windows/Pet/Win32MouseCompanionVisualState.h"

namespace mousefx::windows {

Win32MouseCompanionRendererInput BuildWin32MouseCompanionRendererInput(
    const Win32MouseCompanionVisualState& state);

} // namespace mousefx::windows
