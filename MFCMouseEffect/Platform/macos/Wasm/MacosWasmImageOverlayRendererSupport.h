#pragma once

#include "Platform/macos/Wasm/MacosWasmImageOverlayRendererCore.h"

#if defined(__APPLE__)
#include <CoreGraphics/CoreGraphics.h>
#ifdef __OBJC__
@class NSString;
#else
struct objc_object;
using NSString = objc_object;
#endif
#endif

#include <cstdint>

namespace mousefx::platform::macos::wasm_image_overlay_support {

CGFloat ClampAlpha(float alpha);
uint32_t ClampDelayMs(uint32_t delayMs);
bool HasMotion(const WasmImageOverlayRequest& request);
NSString* NsPathFromWide(const std::wstring& path);

} // namespace mousefx::platform::macos::wasm_image_overlay_support
