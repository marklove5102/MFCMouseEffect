#include "pch.h"

#include "Platform/macos/Wasm/MacosWasmPathStrokeOverlayRenderer.h"

#include "Platform/macos/Effects/MacosOverlayRenderSupport.h"
#include "Platform/macos/Wasm/MacosWasmOverlayRuntime.h"
#include "Platform/macos/Wasm/MacosWasmPathStrokeOverlaySwiftBridge.h"

#if defined(__APPLE__)
#include <dispatch/dispatch.h>
#endif

#include <algorithm>
#include <memory>

namespace mousefx::platform::macos {

#if defined(__APPLE__)
namespace {

constexpr uint32_t kWasmPathStrokeClosePaddingMs = 60u;

struct WasmPathStrokeOverlayCloseContext final {
    void* windowHandle = nullptr;
};

void CloseWasmPathStrokeOverlayAfterDelay(void* context) {
    std::unique_ptr<WasmPathStrokeOverlayCloseContext> closeContext(
        static_cast<WasmPathStrokeOverlayCloseContext*>(context));
    if (!closeContext || closeContext->windowHandle == nullptr) {
        return;
    }
    if (!TakeWasmOverlayWindow(closeContext->windowHandle)) {
        return;
    }
    macos_overlay_support::ReleaseOverlayWindow(closeContext->windowHandle);
}

void RenderWasmPathStrokeOverlayWindowOnMain(const WasmPathStrokeOverlayRequest& request) {
    if (request.nodes.empty()) {
        ReleaseWasmOverlaySlot();
        return;
    }

    RecordWasmPathStrokeOverlayRenderRequest();
    void* windowHandle = mfx_macos_wasm_path_stroke_overlay_create_v1(
        static_cast<double>(request.frameLeftPx),
        static_cast<double>(request.frameTopPx),
        static_cast<double>(request.squareSizePx),
        request.nodes.data(),
        static_cast<uint32_t>(request.nodes.size()),
        static_cast<double>(request.lineWidthPx),
        request.strokeArgb,
        request.glowArgb,
        static_cast<double>(std::clamp(request.alpha, 0.0f, 1.0f)),
        static_cast<double>(std::max<uint32_t>(request.lifeMs, 40u)) / 1000.0,
        request.lineJoin,
        request.lineCap,
        static_cast<uint32_t>(request.semantics.blendMode),
        request.semantics.sortKey,
        request.semantics.groupId,
        static_cast<double>(request.semantics.clipRect.leftPx),
        static_cast<double>(request.semantics.clipRect.topPx),
        static_cast<double>(request.semantics.clipRect.widthPx),
        static_cast<double>(request.semantics.clipRect.heightPx));
    if (windowHandle == nullptr) {
        ReleaseWasmOverlaySlot();
        return;
    }

    RegisterWasmOverlayWindow(windowHandle);
    mfx_macos_wasm_path_stroke_overlay_show_v1(windowHandle);

    auto* closeContext = new WasmPathStrokeOverlayCloseContext{};
    closeContext->windowHandle = windowHandle;
    dispatch_after_f(
        dispatch_time(
            DISPATCH_TIME_NOW,
            static_cast<int64_t>(request.lifeMs + kWasmPathStrokeClosePaddingMs) * NSEC_PER_MSEC),
        dispatch_get_main_queue(),
        closeContext,
        &CloseWasmPathStrokeOverlayAfterDelay);
}

} // namespace
#endif

WasmOverlayRenderResult RenderWasmPathStrokeOverlay(const WasmPathStrokeOverlayRequest& request) {
#if !defined(__APPLE__)
    (void)request;
    return WasmOverlayRenderResult::Failed;
#else
    if (request.nodes.empty()) {
        return WasmOverlayRenderResult::Failed;
    }

    const WasmOverlayAdmissionResult admission = TryAcquireWasmOverlaySlot(WasmOverlayKind::Image);
    if (admission != WasmOverlayAdmissionResult::Accepted) {
        return (admission == WasmOverlayAdmissionResult::RejectedByCapacity)
            ? WasmOverlayRenderResult::ThrottledByCapacity
            : WasmOverlayRenderResult::ThrottledByInterval;
    }

    const WasmPathStrokeOverlayRequest copied = request;
    dispatch_after(
        dispatch_time(DISPATCH_TIME_NOW, static_cast<int64_t>(request.delayMs) * NSEC_PER_MSEC),
        dispatch_get_main_queue(),
        ^{
          RenderWasmPathStrokeOverlayWindowOnMain(copied);
        });
    return WasmOverlayRenderResult::Rendered;
#endif
}

} // namespace mousefx::platform::macos
