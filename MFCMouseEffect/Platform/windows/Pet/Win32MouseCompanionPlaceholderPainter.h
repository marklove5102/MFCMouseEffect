#pragma once

#include <gdiplus.h>

#include "Platform/windows/Pet/Win32MouseCompanionPlaceholderScene.h"

namespace mousefx::windows {

class Win32MouseCompanionPlaceholderPainter final {
public:
    void Paint(
        const Win32MouseCompanionPlaceholderScene& scene,
        Gdiplus::Graphics* graphics,
        int width,
        int height) const;
};

} // namespace mousefx::windows
