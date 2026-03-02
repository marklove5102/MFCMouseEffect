#include "pch.h"

#include "Platform/PlatformInputServicesFactory.h"

#include "Platform/PlatformTarget.h"

#if MFX_PLATFORM_WINDOWS
#include "Platform/windows/System/Win32CursorPositionService.h"
#include "Platform/windows/System/Win32GlobalMouseHook.h"
#include "Platform/windows/System/Win32KeyboardInjector.h"
#elif MFX_PLATFORM_MACOS
#include "Platform/macos/System/MacosCursorPositionService.h"
#include "Platform/macos/System/MacosGlobalInputHook.h"
#include "Platform/macos/System/MacosKeyboardInjector.h"
#else
#include "MouseFx/Core/System/NullCursorPositionService.h"
#include "MouseFx/Core/System/NullGlobalMouseHook.h"
#include "MouseFx/Core/System/NullKeyboardInjector.h"
#endif

namespace mousefx::platform {

std::unique_ptr<IGlobalMouseHook> CreateGlobalMouseHook() {
#if MFX_PLATFORM_WINDOWS
    return std::make_unique<Win32GlobalMouseHook>();
#elif MFX_PLATFORM_MACOS
    return std::make_unique<MacosGlobalInputHook>();
#else
    return std::make_unique<NullGlobalMouseHook>();
#endif
}

std::unique_ptr<ICursorPositionService> CreateCursorPositionService() {
#if MFX_PLATFORM_WINDOWS
    return std::make_unique<Win32CursorPositionService>();
#elif MFX_PLATFORM_MACOS
    return std::make_unique<MacosCursorPositionService>();
#else
    return std::make_unique<NullCursorPositionService>();
#endif
}

std::unique_ptr<IKeyboardInjector> CreateKeyboardInjector() {
#if MFX_PLATFORM_WINDOWS
    return std::make_unique<Win32KeyboardInjector>();
#elif MFX_PLATFORM_MACOS
    return std::make_unique<MacosKeyboardInjector>();
#else
    return std::make_unique<NullKeyboardInjector>();
#endif
}

} // namespace mousefx::platform
