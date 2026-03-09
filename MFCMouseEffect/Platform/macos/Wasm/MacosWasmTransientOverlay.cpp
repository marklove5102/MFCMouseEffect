#include "pch.h"

#include "Platform/macos/Wasm/MacosWasmTransientOverlay.h"

#include "MouseFx/Core/Overlay/OverlayCoordSpace.h"
#include "Platform/macos/Effects/MacosClickPulseOverlayRenderer.h"
#include "Platform/macos/Wasm/MacosWasmGlowBatchOverlayRenderer.h"
#include "Platform/macos/Wasm/MacosWasmImageOverlayRenderer.h"
#include "Platform/macos/Wasm/MacosWasmPathStrokeOverlayRenderer.h"
#include "Platform/macos/Wasm/MacosWasmPolylineOverlayRenderer.h"
#include "Platform/macos/Wasm/MacosWasmOverlayRuntime.h"
#include "Platform/macos/Wasm/MacosWasmSpriteBatchOverlayRenderer.h"
#include "Platform/macos/Wasm/MacosWasmTextOverlayFallback.h"

#if defined(__APPLE__)
#include <dispatch/dispatch.h>
#endif

namespace mousefx::platform::macos {

#if defined(__APPLE__)
namespace {

constexpr uint32_t kWasmPulseClosePaddingMs = 60u;

uint32_t ResolveWasmPulseLifeMs(uint32_t lifeMs) {
    return std::clamp<uint32_t>(lifeMs > 0u ? lifeMs : 320u, 80u, 8000u);
}

int ResolveWasmPulseSizePx(const WasmPulseOverlayRequest& request) {
    if (request.sizePx > 0) {
        return std::clamp(request.sizePx, 32, 640);
    }

    const float endRadiusPx = std::clamp(request.endRadiusPx, 1.0f, 800.0f);
    const float strokeWidthPx = std::clamp(request.strokeWidthPx, 0.1f, 64.0f);
    const float diameter = endRadiusPx * 2.0f;
    const float padding = std::max(18.0f, strokeWidthPx * 6.0f);
    return std::clamp<int>(
        static_cast<int>(std::ceil(diameter + padding)),
        32,
        640);
}

void ReleaseWasmPulseOverlaySlotAfterDelay(void*) {
    ReleaseWasmOverlaySlot();
}

void ShowWasmPulseOverlayOnMain(const WasmPulseOverlayRequest& request) {
    mousefx::ClickEffectRenderCommand command{};
    command.overlayPoint = ScreenToOverlayPoint(request.screenPt);
    command.button = mousefx::MouseButton::Left;
    command.normalizedType = request.normalizedType.empty() ? "ripple" : request.normalizedType;
    command.sizePx = ResolveWasmPulseSizePx(request);
    command.animationDurationSec = static_cast<double>(ResolveWasmPulseLifeMs(request.lifeMs)) / 1000.0;
    command.closePaddingMs = static_cast<int>(kWasmPulseClosePaddingMs);
    command.baseOpacity = std::clamp(static_cast<double>(request.alpha), 0.0, 1.0);
    command.startRadiusPx = std::clamp(static_cast<double>(request.startRadiusPx), 0.0, 640.0);
    command.endRadiusPx = std::clamp(static_cast<double>(request.endRadiusPx), 1.0, 800.0);
    command.endRadiusPx = std::max(command.endRadiusPx, command.startRadiusPx + 1.0);
    command.strokeWidthPx = std::clamp(static_cast<double>(request.strokeWidthPx), 0.1, 64.0);
    command.fillArgb = request.fillArgb;
    command.strokeArgb = request.strokeArgb;
    command.glowArgb = request.glowArgb;
    mousefx::macos_click_pulse::ShowClickPulseOverlay(command, std::string{});
}

struct DelayedWasmPulseOverlayContext final {
    WasmPulseOverlayRequest request{};
};

void ShowDelayedWasmPulseOverlayCallback(void* opaque) {
    std::unique_ptr<DelayedWasmPulseOverlayContext> context(
        static_cast<DelayedWasmPulseOverlayContext*>(opaque));
    if (!context) {
        return;
    }
    ShowWasmPulseOverlayOnMain(context->request);
}

} // namespace
#endif

WasmOverlayRenderResult ShowWasmImageOverlay(const WasmImageOverlayRequest& request) {
    return RenderWasmImageOverlay(request);
}

WasmOverlayRenderResult ShowWasmPolylineOverlay(const WasmPolylineOverlayRequest& request) {
    return RenderWasmPolylineOverlay(request);
}

WasmOverlayRenderResult ShowWasmPathStrokeOverlay(const WasmPathStrokeOverlayRequest& request) {
    return RenderWasmPathStrokeOverlay(request);
}

WasmOverlayRenderResult ShowWasmGlowBatchOverlay(const WasmGlowBatchOverlayRequest& request) {
    return RenderWasmGlowBatchOverlay(request);
}

WasmOverlayRenderResult ShowWasmSpriteBatchOverlay(const WasmSpriteBatchOverlayRequest& request) {
    return RenderWasmSpriteBatchOverlay(request);
}

WasmOverlayRenderResult ShowWasmPulseOverlay(const WasmPulseOverlayRequest& request) {
#if !defined(__APPLE__)
    (void)request;
    return WasmOverlayRenderResult::Failed;
#else
    const WasmOverlayAdmissionResult admission = TryAcquireWasmOverlaySlot(WasmOverlayKind::Image);
    if (admission != WasmOverlayAdmissionResult::Accepted) {
        return (admission == WasmOverlayAdmissionResult::RejectedByCapacity)
            ? WasmOverlayRenderResult::ThrottledByCapacity
            : WasmOverlayRenderResult::ThrottledByInterval;
    }

    const uint32_t lifeMs = ResolveWasmPulseLifeMs(request.lifeMs);
    const uint32_t delayMs = std::min<uint32_t>(request.delayMs, 60000u);
    const uint64_t closeDelayNs = static_cast<uint64_t>(delayMs + lifeMs + kWasmPulseClosePaddingMs) * NSEC_PER_MSEC;
    RecordWasmPulseOverlayRenderRequest();
    RunWasmOverlayOnMainThreadAsync([request, delayMs, closeDelayNs, lifeMs] {
        WasmPulseOverlayRequest resolved = request;
        resolved.lifeMs = lifeMs;
        if (delayMs == 0u) {
            ShowWasmPulseOverlayOnMain(resolved);
        } else {
            auto* context = new DelayedWasmPulseOverlayContext{resolved};
            dispatch_after_f(
                dispatch_time(DISPATCH_TIME_NOW, static_cast<int64_t>(delayMs) * NSEC_PER_MSEC),
                dispatch_get_main_queue(),
                context,
                &ShowDelayedWasmPulseOverlayCallback);
        }

        dispatch_after_f(
            dispatch_time(DISPATCH_TIME_NOW, static_cast<int64_t>(closeDelayNs)),
            dispatch_get_main_queue(),
            nullptr,
            &ReleaseWasmPulseOverlaySlotAfterDelay);
    });
    return WasmOverlayRenderResult::Rendered;
#endif
}

WasmOverlayRenderResult ShowWasmImagePulseOverlay(
    const ScreenPoint& screenPt,
    uint32_t tintArgb,
    float scale,
    float alpha,
    uint32_t lifeMs) {
    WasmImageOverlayRequest request{};
    request.screenPt = screenPt;
    request.tintArgb = tintArgb;
    request.scale = scale;
    request.alpha = alpha;
    request.lifeMs = lifeMs;
    return ShowWasmImageOverlay(request);
}

void CloseAllWasmOverlays() {
#if !defined(__APPLE__)
    return;
#else
    wasm_text_overlay::SharedFallback().Shutdown();
    CloseAllWasmOverlayWindows();
#endif
}

} // namespace mousefx::platform::macos
