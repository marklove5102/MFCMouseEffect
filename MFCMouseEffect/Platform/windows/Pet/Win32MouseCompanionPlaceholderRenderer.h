#pragma once

#include <gdiplus.h>

#include "Platform/windows/Pet/Win32MouseCompanionVisualState.h"

namespace mousefx::windows {

class Win32MouseCompanionPlaceholderRenderer final {
public:
    void Render(
        const Win32MouseCompanionVisualState& state,
        Gdiplus::Graphics* graphics,
        int width,
        int height) const;
};

} // namespace mousefx::windows
