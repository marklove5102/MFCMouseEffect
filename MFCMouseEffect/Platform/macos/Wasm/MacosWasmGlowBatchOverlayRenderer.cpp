#include "pch.h"

#include "Platform/macos/Wasm/MacosWasmGlowBatchOverlayRenderer.h"

#include "Platform/macos/Effects/MacosOverlayRenderSupport.h"
#include "Platform/macos/Wasm/MacosWasmGlowBatchOverlaySwiftBridge.h"
#include "Platform/macos/Wasm/MacosWasmOverlayRuntime.h"

#if defined(__APPLE__)
#include <dispatch/dispatch.h>
#endif

#include <algorithm>
#include <memory>

namespace mousefx::platform::macos {

#if defined(__APPLE__)
namespace {

constexpr uint32_t kWasmGlowBatchClosePaddingMs = 80u;

struct WasmGlowBatchOverlayCloseContext final {
    void* windowHandle = nullptr;
};

void CloseWasmGlowBatchOverlayAfterDelay(void* context) {
    std::unique_ptr<WasmGlowBatchOverlayCloseContext> closeContext(
        static_cast<WasmGlowBatchOverlayCloseContext*>(context));
    if (!closeContext || closeContext->windowHandle == nullptr) {
        return;
    }
    if (!TakeWasmOverlayWindow(closeContext->windowHandle)) {
        return;
    }
    macos_overlay_support::ReleaseOverlayWindow(closeContext->windowHandle);
}

void RenderWasmGlowBatchOverlayWindowOnMain(const WasmGlowBatchOverlayRequest& request) {
    if (request.particles.empty()) {
        ReleaseWasmOverlaySlot();
        return;
    }

    RecordWasmGlowBatchOverlayRenderRequest();
    void* windowHandle = mfx_macos_wasm_glow_batch_overlay_create_v1(
        static_cast<double>(request.frameLeftPx),
        static_cast<double>(request.frameTopPx),
        static_cast<double>(request.squareSizePx),
        request.particles.data(),
        static_cast<uint32_t>(request.particles.size()),
        static_cast<double>(std::max<uint32_t>(request.lifeMs, 60u)) / 1000.0,
        static_cast<uint32_t>(request.semantics.blendMode),
        request.semantics.sortKey,
        request.semantics.groupId);
    if (windowHandle == nullptr) {
        ReleaseWasmOverlaySlot();
        return;
    }

    RegisterWasmOverlayWindow(windowHandle);
    mfx_macos_wasm_glow_batch_overlay_show_v1(windowHandle);

    auto* closeContext = new WasmGlowBatchOverlayCloseContext{};
    closeContext->windowHandle = windowHandle;
    dispatch_after_f(
        dispatch_time(
            DISPATCH_TIME_NOW,
            static_cast<int64_t>(request.lifeMs + kWasmGlowBatchClosePaddingMs) * NSEC_PER_MSEC),
        dispatch_get_main_queue(),
        closeContext,
        &CloseWasmGlowBatchOverlayAfterDelay);
}

} // namespace
#endif

WasmOverlayRenderResult RenderWasmGlowBatchOverlay(const WasmGlowBatchOverlayRequest& request) {
#if !defined(__APPLE__)
    (void)request;
    return WasmOverlayRenderResult::Failed;
#else
    if (request.particles.empty()) {
        return WasmOverlayRenderResult::Failed;
    }

    const WasmOverlayAdmissionResult admission = TryAcquireWasmOverlaySlot(WasmOverlayKind::Image);
    if (admission != WasmOverlayAdmissionResult::Accepted) {
        return (admission == WasmOverlayAdmissionResult::RejectedByCapacity)
            ? WasmOverlayRenderResult::ThrottledByCapacity
            : WasmOverlayRenderResult::ThrottledByInterval;
    }

    const WasmGlowBatchOverlayRequest copied = request;
    dispatch_after(
        dispatch_time(DISPATCH_TIME_NOW, static_cast<int64_t>(request.delayMs) * NSEC_PER_MSEC),
        dispatch_get_main_queue(),
        ^{
          RenderWasmGlowBatchOverlayWindowOnMain(copied);
        });
    return WasmOverlayRenderResult::Rendered;
#endif
}

} // namespace mousefx::platform::macos
