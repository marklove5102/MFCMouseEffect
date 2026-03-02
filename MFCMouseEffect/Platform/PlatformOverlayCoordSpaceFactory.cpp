#include "pch.h"

#include "Platform/PlatformOverlayCoordSpaceFactory.h"
#include "Platform/PlatformTarget.h"

#if MFX_PLATFORM_WINDOWS
#include "Platform/windows/Overlay/Win32OverlayCoordSpaceService.h"
#elif MFX_PLATFORM_MACOS
#include "Platform/macos/Overlay/MacosOverlayCoordSpaceService.h"
#endif

namespace mousefx::platform {

std::unique_ptr<IOverlayCoordSpaceService> CreateOverlayCoordSpaceService() {
#if MFX_PLATFORM_WINDOWS
    return std::make_unique<Win32OverlayCoordSpaceService>();
#elif MFX_PLATFORM_MACOS
    return std::make_unique<MacosOverlayCoordSpaceService>();
#else
    return nullptr;
#endif
}

} // namespace mousefx::platform
