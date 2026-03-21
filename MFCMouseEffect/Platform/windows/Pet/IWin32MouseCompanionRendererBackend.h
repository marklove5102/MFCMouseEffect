#pragma once

#include <gdiplus.h>
#include <string>

#include "Platform/windows/Pet/Win32MouseCompanionRendererInput.h"

namespace mousefx::windows {

class IWin32MouseCompanionRendererBackend {
public:
    virtual ~IWin32MouseCompanionRendererBackend() = default;

    virtual bool Start() = 0;
    virtual void Shutdown() = 0;
    virtual bool IsReady() const = 0;
    virtual std::string LastErrorReason() const = 0;
    virtual void Render(
        const Win32MouseCompanionRendererInput& input,
        Gdiplus::Graphics* graphics,
        int width,
        int height) const = 0;
};

} // namespace mousefx::windows
