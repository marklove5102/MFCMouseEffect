#pragma once

#include "Platform/windows/Pet/IWin32MouseCompanionRendererBackend.h"

namespace mousefx::windows {

class Win32MouseCompanionPlaceholderRenderer final : public IWin32MouseCompanionRendererBackend {
public:
    void Render(
        const Win32MouseCompanionRendererInput& input,
        Gdiplus::Graphics* graphics,
        int width,
        int height) const override;
};

void RegisterWin32MouseCompanionPlaceholderRendererBackend();

} // namespace mousefx::windows
