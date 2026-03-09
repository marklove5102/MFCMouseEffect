#include "pch.h"

#include "Platform/macos/Wasm/MacosWasmPolylineOverlayRenderer.h"

#include "Platform/macos/Effects/MacosOverlayRenderSupport.h"
#include "Platform/macos/Wasm/MacosWasmOverlayRuntime.h"
#include "Platform/macos/Wasm/MacosWasmPolylineOverlaySwiftBridge.h"

#if defined(__APPLE__)
#include <dispatch/dispatch.h>
#endif

#include <algorithm>
#include <memory>

namespace mousefx::platform::macos {

#if defined(__APPLE__)
namespace {

constexpr uint32_t kWasmPolylineClosePaddingMs = 60u;

struct WasmPolylineOverlayCloseContext final {
    void* windowHandle = nullptr;
};

void CloseWasmPolylineOverlayAfterDelay(void* context) {
    std::unique_ptr<WasmPolylineOverlayCloseContext> closeContext(
        static_cast<WasmPolylineOverlayCloseContext*>(context));
    if (!closeContext || closeContext->windowHandle == nullptr) {
        return;
    }
    if (!TakeWasmOverlayWindow(closeContext->windowHandle)) {
        return;
    }
    macos_overlay_support::ReleaseOverlayWindow(closeContext->windowHandle);
}

void RenderWasmPolylineOverlayWindowOnMain(const WasmPolylineOverlayRequest& request) {
    if (request.localPointsXY.size() < 4u) {
        ReleaseWasmOverlaySlot();
        return;
    }

    RecordWasmPolylineOverlayRenderRequest();
    void* windowHandle = mfx_macos_wasm_polyline_overlay_create_v1(
        static_cast<double>(request.frameLeftPx),
        static_cast<double>(request.frameTopPx),
        static_cast<double>(request.squareSizePx),
        request.localPointsXY.data(),
        static_cast<uint32_t>(request.localPointsXY.size() / 2u),
        static_cast<double>(request.lineWidthPx),
        request.strokeArgb,
        request.glowArgb,
        static_cast<double>(std::clamp(request.alpha, 0.0f, 1.0f)),
        static_cast<double>(std::max<uint32_t>(request.lifeMs, 40u)) / 1000.0,
        request.closed ? 1 : 0);
    if (windowHandle == nullptr) {
        ReleaseWasmOverlaySlot();
        return;
    }

    RegisterWasmOverlayWindow(windowHandle);
    mfx_macos_wasm_polyline_overlay_show_v1(windowHandle);

    auto* closeContext = new WasmPolylineOverlayCloseContext{};
    closeContext->windowHandle = windowHandle;
    dispatch_after_f(
        dispatch_time(
            DISPATCH_TIME_NOW,
            static_cast<int64_t>(request.lifeMs + kWasmPolylineClosePaddingMs) * NSEC_PER_MSEC),
        dispatch_get_main_queue(),
        closeContext,
        &CloseWasmPolylineOverlayAfterDelay);
}

} // namespace
#endif

WasmOverlayRenderResult RenderWasmPolylineOverlay(const WasmPolylineOverlayRequest& request) {
#if !defined(__APPLE__)
    (void)request;
    return WasmOverlayRenderResult::Failed;
#else
    if (request.localPointsXY.size() < 4u) {
        return WasmOverlayRenderResult::Failed;
    }

    const WasmOverlayAdmissionResult admission = TryAcquireWasmOverlaySlot(WasmOverlayKind::Image);
    if (admission != WasmOverlayAdmissionResult::Accepted) {
        return (admission == WasmOverlayAdmissionResult::RejectedByCapacity)
            ? WasmOverlayRenderResult::ThrottledByCapacity
            : WasmOverlayRenderResult::ThrottledByInterval;
    }

    const WasmPolylineOverlayRequest copied = request;
    dispatch_after(
        dispatch_time(DISPATCH_TIME_NOW, static_cast<int64_t>(request.delayMs) * NSEC_PER_MSEC),
        dispatch_get_main_queue(),
        ^{
          RenderWasmPolylineOverlayWindowOnMain(copied);
        });
    return WasmOverlayRenderResult::Rendered;
#endif
}

} // namespace mousefx::platform::macos
