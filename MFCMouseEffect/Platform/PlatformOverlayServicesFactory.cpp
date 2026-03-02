#include "pch.h"

#include "Platform/PlatformOverlayServicesFactory.h"

#include "Platform/PlatformTarget.h"

#if MFX_PLATFORM_WINDOWS
#include "Platform/windows/Overlay/Win32OverlayHostBackend.h"
#include "Platform/windows/Overlay/Win32InputIndicatorOverlay.h"
#elif MFX_PLATFORM_MACOS
#include "Platform/macos/Overlay/MacosInputIndicatorOverlay.h"
#else
#include "MouseFx/Core/Overlay/NullInputIndicatorOverlay.h"
#endif

namespace mousefx::platform {

std::unique_ptr<IOverlayHostBackend> CreateOverlayHostBackend() {
#if MFX_PLATFORM_WINDOWS
    return std::make_unique<Win32OverlayHostBackend>();
#else
    return nullptr;
#endif
}

std::unique_ptr<IInputIndicatorOverlay> CreateInputIndicatorOverlay() {
#if MFX_PLATFORM_WINDOWS
    return std::make_unique<Win32InputIndicatorOverlay>();
#elif MFX_PLATFORM_MACOS
    return std::make_unique<MacosInputIndicatorOverlay>();
#else
    return std::make_unique<NullInputIndicatorOverlay>();
#endif
}

} // namespace mousefx::platform
