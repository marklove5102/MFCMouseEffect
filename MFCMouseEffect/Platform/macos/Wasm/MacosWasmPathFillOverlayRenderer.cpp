#include "pch.h"

#include "Platform/macos/Wasm/MacosWasmPathFillOverlayRenderer.h"

#include "Platform/macos/Effects/MacosOverlayRenderSupport.h"
#include "Platform/macos/Wasm/MacosWasmOverlayRuntime.h"
#include "Platform/macos/Wasm/MacosWasmPathFillOverlaySwiftBridge.h"

#if defined(__APPLE__)
#include <dispatch/dispatch.h>
#endif

#include <algorithm>
#include <memory>

namespace mousefx::platform::macos {

#if defined(__APPLE__)
namespace {

constexpr uint32_t kWasmPathFillClosePaddingMs = 60u;

struct WasmPathFillOverlayCloseContext final {
    void* windowHandle = nullptr;
};

void CloseWasmPathFillOverlayAfterDelay(void* context) {
    std::unique_ptr<WasmPathFillOverlayCloseContext> closeContext(
        static_cast<WasmPathFillOverlayCloseContext*>(context));
    if (!closeContext || closeContext->windowHandle == nullptr) {
        return;
    }
    if (!TakeWasmOverlayWindow(closeContext->windowHandle)) {
        return;
    }
    macos_overlay_support::ReleaseOverlayWindow(closeContext->windowHandle);
}

void RenderWasmPathFillOverlayWindowOnMain(const WasmPathFillOverlayRequest& request) {
    if (request.nodes.empty()) {
        ReleaseWasmOverlaySlot();
        return;
    }

    RecordWasmPathFillOverlayRenderRequest();
    void* windowHandle = mfx_macos_wasm_path_fill_overlay_create_v1(
        static_cast<double>(request.frameLeftPx),
        static_cast<double>(request.frameTopPx),
        static_cast<double>(request.squareSizePx),
        request.nodes.data(),
        static_cast<uint32_t>(request.nodes.size()),
        static_cast<double>(request.glowWidthPx),
        request.fillArgb,
        request.glowArgb,
        static_cast<double>(std::clamp(request.alpha, 0.0f, 1.0f)),
        static_cast<double>(std::max<uint32_t>(request.lifeMs, 40u)) / 1000.0,
        request.fillRule,
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
    mfx_macos_wasm_path_fill_overlay_show_v1(windowHandle);

    auto* closeContext = new WasmPathFillOverlayCloseContext{};
    closeContext->windowHandle = windowHandle;
    dispatch_after_f(
        dispatch_time(
            DISPATCH_TIME_NOW,
            static_cast<int64_t>(request.lifeMs + kWasmPathFillClosePaddingMs) * NSEC_PER_MSEC),
        dispatch_get_main_queue(),
        closeContext,
        &CloseWasmPathFillOverlayAfterDelay);
}

} // namespace
#endif

WasmOverlayRenderResult RenderWasmPathFillOverlay(const WasmPathFillOverlayRequest& request) {
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

    const WasmPathFillOverlayRequest copied = request;
    dispatch_after(
        dispatch_time(DISPATCH_TIME_NOW, static_cast<int64_t>(request.delayMs) * NSEC_PER_MSEC),
        dispatch_get_main_queue(),
        ^{
          RenderWasmPathFillOverlayWindowOnMain(copied);
        });
    return WasmOverlayRenderResult::Rendered;
#endif
}

WasmOverlayRenderResult ShowWasmPathFillOverlay(const WasmPathFillOverlayRequest& request) {
    return RenderWasmPathFillOverlay(request);
}

} // namespace mousefx::platform::macos
