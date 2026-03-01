#pragma once

#if defined(__APPLE__)
#include <CoreGraphics/CoreGraphics.h>
#ifdef __OBJC__
@class NSColor;
#else
struct objc_object;
using NSColor = objc_object;
#endif
#endif

#include <cstdint>

namespace mousefx::platform::macos::wasm_overlay_render_math {

CGFloat ClampFloat(CGFloat value, CGFloat lo, CGFloat hi);
CGFloat ClampScale(float scale);
uint32_t ClampLifeMs(uint32_t lifeMs);
NSColor* ColorFromArgb(uint32_t argb, CGFloat alphaScale);

} // namespace mousefx::platform::macos::wasm_overlay_render_math
