#pragma once

#include "Platform/windows/Pet/IWin32MouseCompanionRendererBackend.h"

namespace mousefx::windows {

class Win32MouseCompanionRealRendererBackend final : public IWin32MouseCompanionRendererBackend {
public:
    bool Start() override;
    void Shutdown() override;
    bool IsReady() const override;
    std::string LastErrorReason() const override;
    void Render(
        const Win32MouseCompanionRendererInput& input,
        Gdiplus::Graphics* graphics,
        int width,
        int height) const override;

private:
    std::string lastErrorReason_{"pending_implementation"};
};

void RegisterWin32MouseCompanionRealRendererBackend();

} // namespace mousefx::windows
