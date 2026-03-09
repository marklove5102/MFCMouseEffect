#include "pch.h"

#include "Platform/macos/Wasm/MacosWasmSpriteBatchOverlayRenderer.h"

#include "MouseFx/Utils/StringUtils.h"
#include "Platform/macos/Effects/MacosOverlayRenderSupport.h"
#include "Platform/macos/Wasm/MacosWasmOverlayRuntime.h"
#include "Platform/macos/Wasm/MacosWasmSpriteBatchOverlaySwiftBridge.h"

#if defined(__APPLE__)
#include <dispatch/dispatch.h>
#endif

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>

namespace mousefx::platform::macos {

#if defined(__APPLE__)
namespace {

constexpr uint32_t kWasmSpriteBatchClosePaddingMs = 80u;
constexpr size_t kWasmSpriteBatchBridgeSpriteBytes = 64u;

struct WasmSpriteBatchOverlayCloseContext final {
    void* windowHandle = nullptr;
};

void StoreFloat(std::vector<uint8_t>* bytes, size_t offset, float value) {
    if (!bytes || offset + sizeof(value) > bytes->size()) {
        return;
    }
    std::memcpy(bytes->data() + offset, &value, sizeof(value));
}

void StoreUInt32(std::vector<uint8_t>* bytes, size_t offset, uint32_t value) {
    if (!bytes || offset + sizeof(value) > bytes->size()) {
        return;
    }
    std::memcpy(bytes->data() + offset, &value, sizeof(value));
}

void CloseWasmSpriteBatchOverlayAfterDelay(void* context) {
    std::unique_ptr<WasmSpriteBatchOverlayCloseContext> closeContext(
        static_cast<WasmSpriteBatchOverlayCloseContext*>(context));
    if (!closeContext || closeContext->windowHandle == nullptr) {
        return;
    }
    if (!TakeWasmOverlayWindow(closeContext->windowHandle)) {
        return;
    }
    macos_overlay_support::ReleaseOverlayWindow(closeContext->windowHandle);
}

void RenderWasmSpriteBatchOverlayWindowOnMain(const WasmSpriteBatchOverlayRequest& request) {
    if (request.sprites.empty()) {
        ReleaseWasmOverlaySlot();
        return;
    }

    std::vector<uint8_t> spriteBytes(request.sprites.size() * kWasmSpriteBatchBridgeSpriteBytes, 0u);
    std::vector<std::string> imagePathStorage{};
    std::vector<const char*> imagePathPointers{};
    imagePathStorage.reserve(request.sprites.size());
    imagePathPointers.reserve(request.sprites.size());

    for (size_t index = 0; index < request.sprites.size(); ++index) {
        const WasmSpriteBatchOverlaySprite& sprite = request.sprites[index];
        const size_t base = index * kWasmSpriteBatchBridgeSpriteBytes;
        StoreFloat(&spriteBytes, base + 0u, sprite.localX);
        StoreFloat(&spriteBytes, base + 4u, sprite.localY);
        StoreFloat(&spriteBytes, base + 8u, sprite.widthPx);
        StoreFloat(&spriteBytes, base + 12u, sprite.heightPx);
        StoreFloat(&spriteBytes, base + 16u, sprite.alpha);
        StoreFloat(&spriteBytes, base + 20u, sprite.rotationRad);
        StoreUInt32(&spriteBytes, base + 24u, sprite.tintArgb);
        StoreUInt32(&spriteBytes, base + 28u, sprite.applyTint ? 1u : 0u);
        StoreFloat(&spriteBytes, base + 32u, sprite.srcU0);
        StoreFloat(&spriteBytes, base + 36u, sprite.srcV0);
        StoreFloat(&spriteBytes, base + 40u, sprite.srcU1);
        StoreFloat(&spriteBytes, base + 44u, sprite.srcV1);
        StoreFloat(&spriteBytes, base + 48u, sprite.velocityX);
        StoreFloat(&spriteBytes, base + 52u, sprite.velocityY);
        StoreFloat(&spriteBytes, base + 56u, sprite.accelerationX);
        StoreFloat(&spriteBytes, base + 60u, sprite.accelerationY);

        if (!sprite.assetPath.empty()) {
            imagePathStorage.push_back(Utf16ToUtf8(sprite.assetPath.c_str()));
            imagePathPointers.push_back(imagePathStorage.back().c_str());
        } else {
            imagePathPointers.push_back(nullptr);
        }
    }

    RecordWasmSpriteBatchOverlayRenderRequest();
    void* windowHandle = mfx_macos_wasm_sprite_batch_overlay_create_v1(
        static_cast<double>(request.frameLeftPx),
        static_cast<double>(request.frameTopPx),
        static_cast<double>(request.squareSizePx),
        spriteBytes.data(),
        static_cast<uint32_t>(request.sprites.size()),
        imagePathPointers.data(),
        static_cast<double>(std::max<uint32_t>(request.lifeMs, 60u)) / 1000.0,
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
    mfx_macos_wasm_sprite_batch_overlay_show_v1(windowHandle);

    auto* closeContext = new WasmSpriteBatchOverlayCloseContext{};
    closeContext->windowHandle = windowHandle;
    dispatch_after_f(
        dispatch_time(
            DISPATCH_TIME_NOW,
            static_cast<int64_t>(request.lifeMs + kWasmSpriteBatchClosePaddingMs) * NSEC_PER_MSEC),
        dispatch_get_main_queue(),
        closeContext,
        &CloseWasmSpriteBatchOverlayAfterDelay);
}

} // namespace
#endif

WasmOverlayRenderResult RenderWasmSpriteBatchOverlay(const WasmSpriteBatchOverlayRequest& request) {
#if !defined(__APPLE__)
    (void)request;
    return WasmOverlayRenderResult::Failed;
#else
    if (request.sprites.empty()) {
        return WasmOverlayRenderResult::Failed;
    }

    const WasmOverlayAdmissionResult admission = TryAcquireWasmOverlaySlot(WasmOverlayKind::Image);
    if (admission != WasmOverlayAdmissionResult::Accepted) {
        return (admission == WasmOverlayAdmissionResult::RejectedByCapacity)
            ? WasmOverlayRenderResult::ThrottledByCapacity
            : WasmOverlayRenderResult::ThrottledByInterval;
    }

    const WasmSpriteBatchOverlayRequest copied = request;
    dispatch_after(
        dispatch_time(DISPATCH_TIME_NOW, static_cast<int64_t>(request.delayMs) * NSEC_PER_MSEC),
        dispatch_get_main_queue(),
        ^{
          RenderWasmSpriteBatchOverlayWindowOnMain(copied);
        });
    return WasmOverlayRenderResult::Rendered;
#endif
}

} // namespace mousefx::platform::macos
