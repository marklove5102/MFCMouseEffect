#include "pch.h"

#include "Platform/PlatformControlMessageCodecFactory.h"

#include "Platform/PlatformTarget.h"

#if MFX_PLATFORM_WINDOWS
#include "Platform/windows/Control/Win32DispatchMessageCodec.h"
#elif MFX_PLATFORM_MACOS
#include "Platform/macos/Control/MacosDispatchMessageCodec.h"
#else
#include "MouseFx/Core/Control/NullDispatchMessageCodec.h"
#endif

namespace mousefx::platform {

std::unique_ptr<IDispatchMessageCodec> CreateDispatchMessageCodec() {
#if MFX_PLATFORM_WINDOWS
    return std::make_unique<Win32DispatchMessageCodec>();
#elif MFX_PLATFORM_MACOS
    return std::make_unique<MacosDispatchMessageCodec>();
#else
    return std::make_unique<NullDispatchMessageCodec>();
#endif
}

} // namespace mousefx::platform
