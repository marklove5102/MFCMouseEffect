#pragma once

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererScene.h"

namespace mousefx::windows {

class Win32MouseCompanionRealRendererPainter final {
public:
    void Paint(
        const Win32MouseCompanionRealRendererScene& scene,
        Gdiplus::Graphics* graphics,
        int width,
        int height) const;
};

} // namespace mousefx::windows
