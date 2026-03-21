#pragma once

#include <gdiplus.h>

#include "Platform/windows/Pet/Win32MouseCompanionRendererInput.h"

namespace mousefx::windows {

class IWin32MouseCompanionRendererBackend {
public:
    virtual ~IWin32MouseCompanionRendererBackend() = default;

    virtual void Render(
        const Win32MouseCompanionRendererInput& input,
        Gdiplus::Graphics* graphics,
        int width,
        int height) const = 0;
};

} // namespace mousefx::windows
