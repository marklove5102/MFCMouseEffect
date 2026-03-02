#include "pch.h"

#include "MouseFx/Core/System/GdiPlusSession.h"

#include "Platform/PlatformTarget.h"

#if MFX_PLATFORM_WINDOWS
#include <gdiplus.h>
#endif

namespace mousefx {

bool GdiPlusSession::Startup() {
    if (started_) {
        return true;
    }

#if MFX_PLATFORM_WINDOWS
    Gdiplus::GdiplusStartupInput input;
    ULONG_PTR nativeToken = 0;
    if (Gdiplus::GdiplusStartup(&nativeToken, &input, nullptr) != Gdiplus::Ok) {
        token_ = 0;
        return false;
    }

    token_ = static_cast<std::uintptr_t>(nativeToken);
    started_ = true;
    return true;
#else
    token_ = 1;
    started_ = true;
    return true;
#endif
}

void GdiPlusSession::Shutdown() {
    if (!started_) {
        return;
    }

#if MFX_PLATFORM_WINDOWS
    Gdiplus::GdiplusShutdown(static_cast<ULONG_PTR>(token_));
#endif
    token_ = 0;
    started_ = false;
}

} // namespace mousefx
