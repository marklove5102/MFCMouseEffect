#include "pch.h"

#include "Platform/macos/Wasm/MacosWasmTransientOverlay.h"
#include "Platform/macos/Wasm/MacosWasmOverlayRuntime.h"
#include "Platform/macos/Wasm/MacosWasmTextOverlay.Internal.h"
#include "Platform/macos/Wasm/MacosWasmTextOverlaySwiftBridge.h"

#include "MouseFx/Utils/StringUtils.h"

#if defined(__APPLE__)
#include <dispatch/dispatch.h>
#endif

#include <memory>

namespace mousefx::platform::macos {

#if defined(__APPLE__)
namespace {

struct WasmTextOverlayCloseContext final {
    void* panelHandle = nullptr;
};

void CloseWasmTextOverlayAfterDelay(void* context) {
    std::unique_ptr<WasmTextOverlayCloseContext> closeContext(
        static_cast<WasmTextOverlayCloseContext*>(context));
    if (!closeContext || closeContext->panelHandle == nullptr) {
        return;
    }
    if (!TakeWasmOverlayWindow(closeContext->panelHandle)) {
        return;
    }
    mfx_macos_wasm_text_overlay_release_v1(closeContext->panelHandle);
}

} // namespace
#endif

WasmOverlayRenderResult ShowWasmTextOverlay(
    const ScreenPoint& screenPt,
    const std::wstring& text,
    uint32_t argb,
    float scale,
    uint32_t lifeMs) {
#if !defined(__APPLE__)
    (void)screenPt;
    (void)text;
    (void)argb;
    (void)scale;
    (void)lifeMs;
    return WasmOverlayRenderResult::Failed;
#else
    if (text.empty()) {
        return WasmOverlayRenderResult::Failed;
    }

    const std::string utf8Text = Utf16ToUtf8(text.c_str());
    if (utf8Text.empty()) {
        return WasmOverlayRenderResult::Failed;
    }

    const WasmTextOverlayLayout layout = BuildWasmTextOverlayLayout(screenPt, utf8Text.size(), scale, lifeMs);
    const WasmOverlayAdmissionResult admission = TryAcquireWasmOverlaySlot(WasmOverlayKind::Text);
    if (admission != WasmOverlayAdmissionResult::Accepted) {
        return (admission == WasmOverlayAdmissionResult::RejectedByCapacity)
            ? WasmOverlayRenderResult::ThrottledByCapacity
            : WasmOverlayRenderResult::ThrottledByInterval;
    }

    RunWasmOverlayOnMainThreadAsync([=] {
        void* panelHandle = mfx_macos_wasm_text_overlay_create_v1(
            static_cast<double>(layout.frame.origin.x),
            static_cast<double>(layout.frame.origin.y),
            static_cast<double>(layout.frame.size.width),
            static_cast<double>(layout.frame.size.height),
            static_cast<double>(layout.fontSize),
            argb,
            utf8Text.c_str());
        if (panelHandle == nullptr) {
            ReleaseWasmOverlaySlot();
            return;
        }

        RegisterWasmOverlayWindow(panelHandle);
        mfx_macos_wasm_text_overlay_show_v1(panelHandle);

        auto* closeContext = new WasmTextOverlayCloseContext{};
        closeContext->panelHandle = panelHandle;
        dispatch_after_f(
            dispatch_time(DISPATCH_TIME_NOW, static_cast<int64_t>(layout.durationMs) * NSEC_PER_MSEC),
            dispatch_get_main_queue(),
            closeContext,
            &CloseWasmTextOverlayAfterDelay);
    });
    return WasmOverlayRenderResult::Rendered;
#endif
}

} // namespace mousefx::platform::macos
