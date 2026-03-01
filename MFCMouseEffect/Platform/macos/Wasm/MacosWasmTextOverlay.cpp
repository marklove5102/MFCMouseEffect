#include "pch.h"

#include "Platform/macos/Wasm/MacosWasmTransientOverlay.h"
#include "Platform/macos/Wasm/MacosWasmOverlayRuntime.h"
#include "Platform/macos/Wasm/MacosWasmTextOverlayFallback.h"

#if defined(__APPLE__)
#include <dispatch/dispatch.h>
#endif

#include <algorithm>

namespace mousefx::platform::macos {

#if defined(__APPLE__)
namespace {

uint32_t ResolveWasmTextDurationMs(const TextConfig& config) {
    return static_cast<uint32_t>(std::clamp(config.durationMs, 80, 8000));
}

void ReleaseWasmTextOverlaySlotAfterDelay(void*) {
    ReleaseWasmOverlaySlot();
}

TextConfig ResolveWasmTextConfig(const TextConfig& config) {
    TextConfig resolved = config;
    resolved.durationMs = static_cast<int>(ResolveWasmTextDurationMs(config));
    resolved.floatDistance = std::clamp(resolved.floatDistance, 0, 640);
    if (resolved.fontSize <= 0.0f) {
        resolved.fontSize = 8.0f;
    }
    return resolved;
}

} // namespace
#endif

WasmOverlayRenderResult ShowWasmTextOverlay(
    const ScreenPoint& screenPt,
    const std::wstring& text,
    uint32_t argb,
    const TextConfig& textConfig) {
#if !defined(__APPLE__)
    (void)screenPt;
    (void)text;
    (void)argb;
    (void)textConfig;
    return WasmOverlayRenderResult::Failed;
#else
    if (text.empty()) {
        return WasmOverlayRenderResult::Failed;
    }

    const TextConfig resolvedConfig = ResolveWasmTextConfig(textConfig);
    const uint32_t durationMs = ResolveWasmTextDurationMs(resolvedConfig);
    const WasmOverlayAdmissionResult admission = TryAcquireWasmOverlaySlot(WasmOverlayKind::Text);
    if (admission != WasmOverlayAdmissionResult::Accepted) {
        return (admission == WasmOverlayAdmissionResult::RejectedByCapacity)
            ? WasmOverlayRenderResult::ThrottledByCapacity
            : WasmOverlayRenderResult::ThrottledByInterval;
    }

    RunWasmOverlayOnMainThreadAsync([=] {
        auto& fallback = wasm_text_overlay::SharedFallback();
        fallback.EnsureInitialized(8);
        fallback.ShowText(screenPt, text, Argb{argb}, resolvedConfig);

        dispatch_after_f(
            dispatch_time(DISPATCH_TIME_NOW, static_cast<int64_t>(durationMs) * NSEC_PER_MSEC),
            dispatch_get_main_queue(),
            nullptr,
            &ReleaseWasmTextOverlaySlotAfterDelay);
    });
    return WasmOverlayRenderResult::Rendered;
#endif
}

} // namespace mousefx::platform::macos
