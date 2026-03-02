#include "pch.h"

#include "Platform/PlatformControlServicesFactory.h"

#include "Platform/PlatformTarget.h"

#if MFX_PLATFORM_WINDOWS
#include "Platform/windows/Control/Win32DispatchMessageHost.h"
#elif MFX_PLATFORM_MACOS
#include "Platform/macos/Control/MacosDispatchMessageHost.h"
#else
#include "MouseFx/Core/Control/NullDispatchMessageHost.h"
#endif

namespace mousefx::platform {

std::unique_ptr<IDispatchMessageHost> CreateDispatchMessageHost() {
#if MFX_PLATFORM_WINDOWS
    return std::make_unique<Win32DispatchMessageHost>();
#elif MFX_PLATFORM_MACOS
    return std::make_unique<MacosDispatchMessageHost>();
#else
    return std::make_unique<NullDispatchMessageHost>();
#endif
}

} // namespace mousefx::platform
