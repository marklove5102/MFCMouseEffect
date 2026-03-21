#pragma once

#include <windows.h>

#include "Platform/windows/Pet/Win32MouseCompanionVisualState.h"

namespace mousefx::windows {

class Win32MouseCompanionPresenter final {
public:
    RECT ResolveWindowBounds(const Win32MouseCompanionVisualState& state) const;

private:
    void ApplyClampMode(RECT* rect, const Win32MouseCompanionVisualState& state) const;
};

} // namespace mousefx::windows
