#include "pch.h"

#include "MouseFx/Core/Wasm/WasmRetainedQuadFieldRuntime.h"
#include "MouseFx/Core/Wasm/WasmRetainedEmitterRuntimeShared.h"
#include "MouseFx/Core/Wasm/WasmGroupClipRectRuntime.h"
#include "MouseFx/Core/Wasm/WasmGroupEffectiveOffset.h"
#include "MouseFx/Core/Wasm/WasmGroupPassRuntime.h"
#include "MouseFx/Core/Wasm/WasmGroupPassStyle.h"
#include "MouseFx/Core/Wasm/WasmGroupMaterialColor.h"
#include "MouseFx/Core/Wasm/WasmGroupMaterialRuntime.h"
#include "MouseFx/Core/Wasm/WasmGroupMaterialStyle.h"
#include "MouseFx/Core/Wasm/WasmGroupTransformMath.h"
#include "MouseFx/Core/Wasm/WasmGroupLayerRuntime.h"
#include "MouseFx/Core/Wasm/WasmGroupPresentationRuntime.h"

#include "Platform/PlatformTarget.h"

#if MFX_PLATFORM_WINDOWS
#include "MouseFx/Core/Overlay/OverlayHostService.h"
#include "MouseFx/Core/Wasm/WasmSpriteBatchRenderShared.h"
#include "MouseFx/Interfaces/IRippleRenderer.h"
#include "MouseFx/Utils/TimeUtils.h"
#endif

#if MFX_PLATFORM_MACOS
#include "MouseFx/Utils/StringUtils.h"
#include "Platform/macos/Wasm/MacosWasmRetainedQuadFieldSwiftBridge.h"
#endif

#include <algorithm>
#include <cmath>
#include <cstring>
#include <memory>
#include <utility>
#include <vector>

namespace mousefx::wasm {

namespace {

struct RetainedQuadFieldEntry final {
#if MFX_PLATFORM_MACOS
    void* handle = nullptr;
#elif MFX_PLATFORM_WINDOWS
    uint64_t rippleId = 0u;
    int squareSizePx = 0;
    std::shared_ptr<class WindowsRetainedQuadFieldSharedState> state{};
#endif
    uint32_t groupId = 0u;
    mousefx::RenderBlendMode blendMode = mousefx::RenderBlendMode::Normal;
    int32_t sortKey = 0;
    mousefx::RenderClipRect clipRect{};
    bool useGroupLocalOrigin = false;
    ResolvedQuadFieldCommand sourceResolved{};
    int32_t baseFrameLeftPx = 0;
    int32_t baseFrameTopPx = 0;
#if MFX_PLATFORM_WINDOWS
    int32_t baseCenterX = 0;
    int32_t baseCenterY = 0;
#endif
};

using RetainedQuadFieldStore = RetainedEmitterRuntimeStore<RetainedQuadFieldEntry>;

ResolvedQuadFieldCommand ResolveQuadFieldForGroupTransform(
    const ResolvedQuadFieldCommand& source,
    const GroupEffectiveTransform& transform) {
    if (!HasGeometryGroupTransform(source.useGroupLocalOrigin, transform.rotationRad, transform.scaleX, transform.scaleY) ||
        source.sourceItems.empty()) {
        return source;
    }

    ResolvedQuadFieldCommand resolved = source;
    ResolvedSpawnSpriteBatchCommand batch{};
    batch.delayMs = source.batch.delayMs;
    batch.lifeMs = source.batch.lifeMs;
    batch.semantics = source.batch.semantics;
    batch.items.reserve(source.sourceItems.size());

    const float averageScale = AverageGroupTransformScale(transform.scaleX, transform.scaleY);
    const float scaleX = ClampGroupTransformScale(transform.scaleX);
    const float scaleY = ClampGroupTransformScale(transform.scaleY);
    const double lifeSec = static_cast<double>(batch.lifeMs) / 1000.0;

    double screenMinX = 0.0;
    double screenMaxX = 0.0;
    double screenMinY = 0.0;
    double screenMaxY = 0.0;
    bool haveBounds = false;

    auto includePoint = [&](double x, double y) {
        if (!haveBounds) {
            screenMinX = screenMaxX = x;
            screenMinY = screenMaxY = y;
            haveBounds = true;
            return;
        }
        screenMinX = std::min(screenMinX, x);
        screenMaxX = std::max(screenMaxX, x);
        screenMinY = std::min(screenMinY, y);
        screenMaxY = std::max(screenMaxY, y);
    };

    for (const ResolvedSpriteBatchItem& sourceItem : source.sourceItems) {
        const GroupTransformVector transformedPoint = ApplyGroupTransformToPoint(
            sourceItem.localX,
            sourceItem.localY,
            transform.rotationRad,
            transform.scaleX,
            transform.scaleY,
            transform.pivotXPx,
            transform.pivotYPx);
        ResolvedSpriteBatchItem item = sourceItem;
        item.localX = transformedPoint.x;
        item.localY = transformedPoint.y;
        item.widthPx = ClampSpawnSpriteBatchFloat(sourceItem.widthPx * scaleX, sourceItem.widthPx, 8.0f, 420.0f);
        item.heightPx = ClampSpawnSpriteBatchFloat(sourceItem.heightPx * scaleY, sourceItem.heightPx, 8.0f, 420.0f);
        item.rotationRad = sourceItem.rotationRad + transform.rotationRad;
        item.velocityX = sourceItem.velocityX * averageScale;
        item.velocityY = sourceItem.velocityY * averageScale;
        item.accelerationX = sourceItem.accelerationX * averageScale;
        item.accelerationY = sourceItem.accelerationY * averageScale;
        batch.items.push_back(item);

        const double halfWidth = static_cast<double>(item.widthPx) * 0.5;
        const double halfHeight = static_cast<double>(item.heightPx) * 0.5;
        includePoint(static_cast<double>(item.localX) - halfWidth, static_cast<double>(item.localY) - halfHeight);
        includePoint(static_cast<double>(item.localX) + halfWidth, static_cast<double>(item.localY) + halfHeight);

        const double endX = static_cast<double>(item.localX) +
            static_cast<double>(item.velocityX) * lifeSec +
            0.5 * static_cast<double>(item.accelerationX) * lifeSec * lifeSec;
        const double endY = static_cast<double>(item.localY) +
            static_cast<double>(item.velocityY) * lifeSec +
            0.5 * static_cast<double>(item.accelerationY) * lifeSec * lifeSec;
        includePoint(endX - halfWidth, endY - halfHeight);
        includePoint(endX + halfWidth, endY + halfHeight);
    }

    if (!haveBounds) {
        return source;
    }

    const int widthPx = std::max(1, static_cast<int>(std::ceil(screenMaxX - screenMinX)));
    const int heightPx = std::max(1, static_cast<int>(std::ceil(screenMaxY - screenMinY)));
    const int sidePx = std::clamp(std::max(widthPx, heightPx) + 24, 32, 1024);
    const int offsetX = (sidePx - widthPx) / 2;
    const int offsetY = (sidePx - heightPx) / 2;
    batch.frameLeftPx = static_cast<int>(std::floor(screenMinX)) - offsetX;
    batch.frameTopPx = static_cast<int>(std::floor(screenMinY)) - offsetY;
    batch.squareSizePx = sidePx;
    batch.centerScreenPt.x = static_cast<int32_t>(std::lround((screenMinX + screenMaxX) * 0.5));
    batch.centerScreenPt.y = static_cast<int32_t>(std::lround((screenMinY + screenMaxY) * 0.5));

    for (ResolvedSpriteBatchItem& item : batch.items) {
        item.localX -= static_cast<float>(batch.frameLeftPx);
        item.localY -= static_cast<float>(batch.frameTopPx);
    }

    resolved.batch = std::move(batch);
    return resolved;
}

ResolvedQuadFieldCommand ResolveQuadFieldForGroupMaterial(
    const ResolvedQuadFieldCommand& source,
    const GroupMaterialState& materialState) {
    ResolvedQuadFieldCommand resolved = source;
    const GroupMaterialStyleProfile styleProfile = ResolveGroupMaterialStyleProfile(materialState);
    const GroupMaterialEchoVector batchDrift = ResolveGroupMaterialEchoDrift(
        0.0f,
        0.0f,
        0.0f,
        styleProfile.echoDriftPx,
        styleProfile.feedbackMode,
        styleProfile.feedbackPhaseRad,
        styleProfile.feedbackLayerCount,
        styleProfile.feedbackLayerFalloff);
    for (auto& item : resolved.sourceItems) {
        const bool baseApplyTint = item.applyTint;
        item.applyTint = GroupMaterialChangesSpriteTint(baseApplyTint, materialState);
        item.tintArgb = ApplyGroupMaterialToSpriteTint(item.tintArgb, baseApplyTint, materialState);
        item.widthPx = ClampSpawnSpriteBatchFloat(item.widthPx * styleProfile.sizeMultiplier, item.widthPx, 8.0f, 420.0f);
        item.heightPx = ClampSpawnSpriteBatchFloat(item.heightPx * styleProfile.sizeMultiplier, item.heightPx, 8.0f, 420.0f);
        item.alpha = std::clamp(item.alpha * styleProfile.alphaMultiplier, 0.0f, 1.0f);
        const GroupMaterialEchoVector itemDrift = ResolveGroupMaterialEchoDrift(
            item.velocityX,
            item.velocityY,
            item.rotationRad,
            styleProfile.echoDriftPx,
            styleProfile.feedbackMode,
            styleProfile.feedbackPhaseRad,
            styleProfile.feedbackLayerCount,
            styleProfile.feedbackLayerFalloff);
        item.localX += itemDrift.x;
        item.localY += itemDrift.y;
    }
    for (auto& item : resolved.batch.items) {
        const bool baseApplyTint = item.applyTint;
        item.applyTint = GroupMaterialChangesSpriteTint(baseApplyTint, materialState);
        item.tintArgb = ApplyGroupMaterialToSpriteTint(item.tintArgb, baseApplyTint, materialState);
        item.widthPx = ClampSpawnSpriteBatchFloat(item.widthPx * styleProfile.sizeMultiplier, item.widthPx, 8.0f, 420.0f);
        item.heightPx = ClampSpawnSpriteBatchFloat(item.heightPx * styleProfile.sizeMultiplier, item.heightPx, 8.0f, 420.0f);
        item.alpha = std::clamp(item.alpha * styleProfile.alphaMultiplier, 0.0f, 1.0f);
        const GroupMaterialEchoVector itemDrift = ResolveGroupMaterialEchoDrift(
            item.velocityX,
            item.velocityY,
            item.rotationRad,
            styleProfile.echoDriftPx,
            styleProfile.feedbackMode,
            styleProfile.feedbackPhaseRad,
            styleProfile.feedbackLayerCount,
            styleProfile.feedbackLayerFalloff);
        item.localX += itemDrift.x;
        item.localY += itemDrift.y;
    }
    resolved.ttlMs = std::clamp<uint32_t>(
        static_cast<uint32_t>(std::lround(static_cast<double>(source.ttlMs) * styleProfile.ttlMultiplier)),
        40u,
        15000u);
    resolved.batch.lifeMs = resolved.ttlMs;
    resolved.batch.centerScreenPt.x = static_cast<int32_t>(std::lround(static_cast<double>(resolved.batch.centerScreenPt.x) + batchDrift.x));
    resolved.batch.centerScreenPt.y = static_cast<int32_t>(std::lround(static_cast<double>(resolved.batch.centerScreenPt.y) + batchDrift.y));
    return resolved;
}

ResolvedQuadFieldCommand ResolveQuadFieldForGroupPass(
    const ResolvedQuadFieldCommand& source,
    const GroupPassState& passState) {
    ResolvedQuadFieldCommand resolved = source;
    const GroupPassStyleProfile passProfile = ResolveGroupPassStyleProfileForLane(
        passState,
        kGroupPassRouteQuad);
    const GroupPassEchoVector batchDrift = ResolveGroupPassEchoDrift(
        0.0f,
        0.0f,
        0.0f,
        passProfile.echoDriftPx,
        passProfile.passMode,
        passProfile.phaseRad,
        passProfile.feedbackLayerCount,
        passProfile.feedbackLayerFalloff);
    for (auto& item : resolved.sourceItems) {
        item.widthPx = ClampSpawnSpriteBatchFloat(item.widthPx * passProfile.sizeMultiplier, item.widthPx, 8.0f, 420.0f);
        item.heightPx = ClampSpawnSpriteBatchFloat(item.heightPx * passProfile.sizeMultiplier, item.heightPx, 8.0f, 420.0f);
        item.alpha = std::clamp(item.alpha * passProfile.alphaMultiplier, 0.0f, 1.0f);
        const GroupPassEchoVector itemDrift = ResolveGroupPassEchoDrift(
            item.velocityX,
            item.velocityY,
            item.rotationRad,
            passProfile.echoDriftPx,
            passProfile.passMode,
            passProfile.phaseRad,
            passProfile.feedbackLayerCount,
            passProfile.feedbackLayerFalloff);
        item.localX += itemDrift.x;
        item.localY += itemDrift.y;
    }
    for (auto& item : resolved.batch.items) {
        item.widthPx = ClampSpawnSpriteBatchFloat(item.widthPx * passProfile.sizeMultiplier, item.widthPx, 8.0f, 420.0f);
        item.heightPx = ClampSpawnSpriteBatchFloat(item.heightPx * passProfile.sizeMultiplier, item.heightPx, 8.0f, 420.0f);
        item.alpha = std::clamp(item.alpha * passProfile.alphaMultiplier, 0.0f, 1.0f);
        const GroupPassEchoVector itemDrift = ResolveGroupPassEchoDrift(
            item.velocityX,
            item.velocityY,
            item.rotationRad,
            passProfile.echoDriftPx,
            passProfile.passMode,
            passProfile.phaseRad,
            passProfile.feedbackLayerCount,
            passProfile.feedbackLayerFalloff);
        item.localX += itemDrift.x;
        item.localY += itemDrift.y;
    }
    resolved.ttlMs = std::clamp<uint32_t>(
        static_cast<uint32_t>(std::lround(static_cast<double>(source.ttlMs) * passProfile.ttlMultiplier)),
        40u,
        15000u);
    resolved.batch.lifeMs = resolved.ttlMs;
    resolved.batch.centerScreenPt.x = static_cast<int32_t>(std::lround(static_cast<double>(resolved.batch.centerScreenPt.x) + batchDrift.x));
    resolved.batch.centerScreenPt.y = static_cast<int32_t>(std::lround(static_cast<double>(resolved.batch.centerScreenPt.y) + batchDrift.y));
    return resolved;
}

ResolvedQuadFieldCommand ResolveEntryQuadFieldCommand(
    const std::wstring& activeManifestPath,
    const RetainedQuadFieldEntry& entry) {
    return ResolveQuadFieldForGroupTransform(
        ResolveQuadFieldForGroupPass(
            ResolveQuadFieldForGroupMaterial(
                entry.sourceResolved,
                ResolveGroupMaterial(activeManifestPath, entry.groupId)),
            ResolveGroupPass(activeManifestPath, entry.groupId)),
        ResolveEffectiveGroupTransform(activeManifestPath, entry.groupId, entry.useGroupLocalOrigin));
}

#if MFX_PLATFORM_MACOS
bool IsHandleActive(void* handle) {
    return handle != nullptr && mfx_macos_wasm_retained_quad_field_is_active_v1(handle) != 0;
}

void ReleaseHandle(void* handle) {
    if (handle != nullptr) {
        mfx_macos_wasm_retained_quad_field_release_v1(handle);
    }
}

bool CreateHandle(RetainedQuadFieldEntry* entry, const ResolvedQuadFieldCommand& resolved, std::string* outError) {
    if (!entry) {
        if (outError) {
            *outError = "retained quad field entry is null";
        }
        return false;
    }

    void* handle = mfx_macos_wasm_retained_quad_field_create_v1();
    if (handle == nullptr) {
        if (outError) {
            *outError = "failed to create retained quad field handle";
        }
        return false;
    }
    entry->handle = handle;
    entry->groupId = resolved.batch.semantics.groupId;
    return true;
}

void UpsertHandle(void* handle, const ResolvedQuadFieldCommand& resolved) {
    if (handle == nullptr) {
        return;
    }

    constexpr size_t kBridgeSpriteBytes = 64u;
    std::vector<uint8_t> spriteBytes(resolved.batch.items.size() * kBridgeSpriteBytes, 0u);
    std::vector<std::string> imagePathStorage{};
    std::vector<const char*> imagePathPointers{};
    imagePathStorage.reserve(resolved.batch.items.size());
    imagePathPointers.reserve(resolved.batch.items.size());

    auto storeFloat = [&](size_t offset, float value) {
        if (offset + sizeof(value) <= spriteBytes.size()) {
            std::memcpy(spriteBytes.data() + offset, &value, sizeof(value));
        }
    };
    auto storeUInt32 = [&](size_t offset, uint32_t value) {
        if (offset + sizeof(value) <= spriteBytes.size()) {
            std::memcpy(spriteBytes.data() + offset, &value, sizeof(value));
        }
    };

    for (size_t index = 0; index < resolved.batch.items.size(); ++index) {
        const ResolvedSpriteBatchItem& item = resolved.batch.items[index];
        const size_t base = index * kBridgeSpriteBytes;
        storeFloat(base + 0u, item.localX);
        storeFloat(base + 4u, item.localY);
        storeFloat(base + 8u, item.widthPx);
        storeFloat(base + 12u, item.heightPx);
        storeFloat(base + 16u, item.alpha);
        storeFloat(base + 20u, item.rotationRad);
        storeUInt32(base + 24u, item.tintArgb);
        storeUInt32(base + 28u, item.applyTint ? 1u : 0u);
        storeFloat(base + 32u, item.srcU0);
        storeFloat(base + 36u, item.srcV0);
        storeFloat(base + 40u, item.srcU1);
        storeFloat(base + 44u, item.srcV1);
        storeFloat(base + 48u, item.velocityX);
        storeFloat(base + 52u, item.velocityY);
        storeFloat(base + 56u, item.accelerationX);
        storeFloat(base + 60u, item.accelerationY);

        if (!item.assetPath.empty()) {
            imagePathStorage.push_back(Utf16ToUtf8(item.assetPath.c_str()));
            imagePathPointers.push_back(imagePathStorage.back().c_str());
        } else {
            imagePathPointers.push_back(nullptr);
        }
    }

    mfx_macos_wasm_retained_quad_field_upsert_v1(
        handle,
        resolved.batch.frameLeftPx,
        resolved.batch.frameTopPx,
        resolved.batch.squareSizePx,
        spriteBytes.data(),
        static_cast<uint32_t>(resolved.batch.items.size()),
        imagePathPointers.data(),
        resolved.ttlMs,
        static_cast<uint32_t>(resolved.batch.semantics.blendMode),
        resolved.batch.semantics.sortKey,
        resolved.batch.semantics.groupId,
        resolved.batch.semantics.clipRect.leftPx,
        resolved.batch.semantics.clipRect.topPx,
        resolved.batch.semantics.clipRect.widthPx,
        resolved.batch.semantics.clipRect.heightPx);
}

void ApplyHandleGroupPresentation(void* handle, const GroupPresentationState& presentation) {
    if (handle == nullptr) {
        return;
    }
    mfx_macos_wasm_retained_quad_field_set_group_presentation_v1(
        handle,
        presentation.alphaMultiplier,
        presentation.visible ? 1u : 0u);
}

void ApplyHandleGroupClipRect(
    void* handle,
    const mousefx::RenderClipRect& clipRect,
    uint8_t maskShapeKind,
    float cornerRadiusPx) {
    if (handle == nullptr) {
        return;
    }
    mfx_macos_wasm_retained_quad_field_set_effective_clip_rect_v2(
        handle,
        clipRect.leftPx,
        clipRect.topPx,
        clipRect.widthPx,
        clipRect.heightPx,
        static_cast<uint32_t>(maskShapeKind),
        cornerRadiusPx);
}

void ApplyHandleGroupLayer(void* handle, mousefx::RenderBlendMode blendMode, int32_t sortKey) {
    if (handle == nullptr) {
        return;
    }
    mfx_macos_wasm_retained_quad_field_set_effective_layer_v1(
        handle,
        static_cast<uint32_t>(blendMode),
        sortKey);
}

void ApplyHandleGroupTransform(void* handle, float offsetXPx, float offsetYPx) {
    if (handle == nullptr) {
        return;
    }
    mfx_macos_wasm_retained_quad_field_set_effective_translation_v1(handle, offsetXPx, offsetYPx);
}

#elif MFX_PLATFORM_WINDOWS
class WindowsRetainedQuadFieldSharedState final {
public:
    void Upsert(const ResolvedQuadFieldCommand& resolved) {
        std::lock_guard<std::mutex> lock(mutex_);
        batch_ = resolved.batch;
        ttlMs_ = std::max<uint32_t>(40u, resolved.ttlMs);
        startTickMs_ = NowMs();
        expireTickMs_ = startTickMs_ + static_cast<uint64_t>(ttlMs_);
        alive_ = !batch_.items.empty();
    }

    void RequestStop() {
        std::lock_guard<std::mutex> lock(mutex_);
        alive_ = false;
        expireTickMs_ = 0u;
    }

    bool IsAlive() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return alive_;
    }

    void Render(Gdiplus::Graphics& g) {
        std::vector<ResolvedSpriteBatchItem> items;
        RenderBlendMode blendMode = RenderBlendMode::Normal;
        uint32_t ttlMs = 0u;
        uint64_t startTickMs = 0u;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!alive_) {
                return;
            }

            const uint64_t nowMs = NowMs();
            if (nowMs >= expireTickMs_) {
                alive_ = false;
                return;
            }

            items = batch_.items;
            blendMode = batch_.semantics.blendMode;
            ttlMs = ttlMs_;
            startTickMs = startTickMs_;
        }

        if (items.empty() || ttlMs == 0u) {
            return;
        }

        const uint64_t nowMs = NowMs();
        const float elapsedSec = static_cast<float>(nowMs - startTickMs) / 1000.0f;
        const float progress = std::clamp(
            static_cast<float>(nowMs - startTickMs) / static_cast<float>(ttlMs),
            0.0f,
            1.0f);
        const float fade = std::max(0.0f, 1.0f - progress * progress);
        const bool screenBlend = UsesScreenLikeBlend(blendMode);
        g.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
        g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);

        for (const ResolvedSpriteBatchItem& item : items) {
            const float alpha = item.alpha * fade;
            if (alpha <= 0.001f) {
                continue;
            }
            const float x = item.localX +
                item.velocityX * elapsedSec +
                0.5f * item.accelerationX * elapsedSec * elapsedSec;
            const float y = item.localY +
                item.velocityY * elapsedSec +
                0.5f * item.accelerationY * elapsedSec * elapsedSec;
            sprite_batch_render_shared::DrawResolvedSprite(
                g,
                item,
                x,
                y,
                alpha,
                screenBlend,
                &cache_);
        }
    }

private:
    mutable std::mutex mutex_{};
    ResolvedSpawnSpriteBatchCommand batch_{};
    uint32_t ttlMs_ = 640u;
    uint64_t startTickMs_ = 0u;
    uint64_t expireTickMs_ = 0u;
    bool alive_ = false;
    sprite_batch_render_shared::BitmapCache cache_{};
};

class WindowsRetainedQuadFieldRenderer final : public IRippleRenderer {
public:
    explicit WindowsRetainedQuadFieldRenderer(std::shared_ptr<WindowsRetainedQuadFieldSharedState> state)
        : state_(std::move(state)) {}

    void Render(
        Gdiplus::Graphics& g,
        float,
        uint64_t,
        int,
        const RippleStyle&) override {
        if (state_) {
            state_->Render(g);
        }
    }

private:
    std::shared_ptr<WindowsRetainedQuadFieldSharedState> state_{};
};

bool IsHandleActive(const RetainedQuadFieldEntry& entry) {
    return entry.state && entry.state->IsAlive();
}

void ReleaseHandle(RetainedQuadFieldEntry* entry) {
    if (!entry) {
        return;
    }
    if (entry->state) {
        entry->state->RequestStop();
    }
    if (entry->rippleId != 0u) {
        OverlayHostService::Instance().StopRipple(entry->rippleId);
    }
    entry->rippleId = 0u;
    entry->squareSizePx = 0;
    entry->state.reset();
}

bool CreateHandle(RetainedQuadFieldEntry* entry, const ResolvedQuadFieldCommand& resolved, std::string* outError) {
    if (!entry) {
        if (outError) {
            *outError = "retained quad field entry is null";
        }
        return false;
    }

    auto state = std::make_shared<WindowsRetainedQuadFieldSharedState>();
    state->Upsert(resolved);

    ClickEvent ev{};
    ev.button = MouseButton::Left;
    ev.pt = resolved.batch.centerScreenPt;

    RippleStyle style{};
    style.durationMs = 1u;
    style.windowSize = resolved.batch.squareSizePx;
    style.startRadius = 0.0f;
    style.endRadius = 0.0f;
    style.strokeWidth = 0.0f;
    style.fill = {0u};
    style.stroke = {0u};
    style.glow = {0u};

    RenderParams params{};
    params.loop = false;
    params.intensity = 1.0f;
    params.semantics = resolved.batch.semantics;

    const uint64_t rippleId = OverlayHostService::Instance().ShowContinuousRipple(
        ev,
        style,
        std::make_unique<WindowsRetainedQuadFieldRenderer>(state),
        params);
    if (rippleId == 0u) {
        if (outError) {
            *outError = "failed to create retained quad field overlay";
        }
        return false;
    }

    entry->rippleId = rippleId;
    entry->squareSizePx = resolved.batch.squareSizePx;
    entry->state = std::move(state);
    entry->groupId = resolved.batch.semantics.groupId;
    return true;
}

void UpsertHandle(RetainedQuadFieldEntry* entry, const ResolvedQuadFieldCommand& resolved) {
    if (!entry || entry->rippleId == 0u || !entry->state) {
        return;
    }
    entry->state->Upsert(resolved);
    OverlayHostService::Instance().UpdateRipplePosition(entry->rippleId, resolved.batch.centerScreenPt);
}

void ApplyHandleGroupPresentation(RetainedQuadFieldEntry* entry, const GroupPresentationState&) {
    (void)entry;
}

void ApplyHandleGroupClipRect(RetainedQuadFieldEntry* entry, const mousefx::RenderClipRect&, uint8_t, float) {
    (void)entry;
}

void ApplyHandleGroupLayer(RetainedQuadFieldEntry* entry, mousefx::RenderBlendMode, int32_t) {
    (void)entry;
}

void ApplyHandleGroupTransform(RetainedQuadFieldEntry* entry, float offsetXPx, float offsetYPx) {
    if (!entry || entry->rippleId == 0u) {
        return;
    }
    const ScreenPoint translated{
        static_cast<LONG>(std::lround(static_cast<double>(entry->baseCenterX) + static_cast<double>(offsetXPx))),
        static_cast<LONG>(std::lround(static_cast<double>(entry->baseCenterY) + static_cast<double>(offsetYPx))),
    };
    OverlayHostService::Instance().UpdateRipplePosition(entry->rippleId, translated);
}
#endif

GroupEffectiveOffset ResolveEntryGroupOffset(
    const std::wstring& activeManifestPath,
    const RetainedQuadFieldEntry& entry) {
    return ResolveEffectiveGroupOffset(activeManifestPath, entry.groupId, entry.useGroupLocalOrigin);
}

void PruneInactiveRetainedQuadFieldsLocked() {
    ::mousefx::wasm::PruneInactiveRetainedEmittersLocked<RetainedQuadFieldStore>(
        [](const RetainedQuadFieldEntry& entry) {
#if MFX_PLATFORM_MACOS
            return IsHandleActive(entry.handle);
#elif MFX_PLATFORM_WINDOWS
            return IsHandleActive(entry);
#else
            return true;
#endif
        },
        [](RetainedQuadFieldEntry* entry) {
#if MFX_PLATFORM_MACOS
            ReleaseHandle(entry ? entry->handle : nullptr);
#elif MFX_PLATFORM_WINDOWS
            ReleaseHandle(entry);
#else
            (void)entry;
#endif
        });
}

} // namespace

bool UpsertRetainedQuadField(
    const std::wstring& activeManifestPath,
    const ResolvedQuadFieldCommand& resolved,
    std::string* outError) {
#if !(MFX_PLATFORM_MACOS || MFX_PLATFORM_WINDOWS)
    (void)activeManifestPath;
    (void)resolved;
    if (outError) {
        *outError = "retained quad fields are not supported on this platform";
    }
    return false;
#else
    const std::wstring key = BuildRetainedEmitterKey(activeManifestPath, resolved.fieldId);
    if (key.empty()) {
        if (outError) {
            *outError = "retained quad field requires active manifest path and field id";
        }
        return false;
    }

    RetainedQuadFieldStore::UpsertRequests().fetch_add(1, std::memory_order_relaxed);
    const ResolvedQuadFieldCommand effectiveResolved = ResolveQuadFieldForGroupTransform(
        ResolveQuadFieldForGroupMaterial(
            resolved,
            ResolveGroupMaterial(activeManifestPath, resolved.batch.semantics.groupId)),
        ResolveEffectiveGroupTransform(activeManifestPath, resolved.batch.semantics.groupId, resolved.useGroupLocalOrigin));

    std::lock_guard<std::mutex> lock(RetainedQuadFieldStore::Mutex());
    PruneInactiveRetainedQuadFieldsLocked();
    auto& entries = RetainedQuadFieldStore::Entries();
    auto [it, inserted] = entries.try_emplace(key);
    RetainedQuadFieldEntry& entry = it->second;
    if (inserted) {
        if (!CreateHandle(&entry, effectiveResolved, outError)) {
            entries.erase(it);
            return false;
        }
        UpsertHandle(
#if MFX_PLATFORM_MACOS
            entry.handle,
#else
            &entry,
#endif
            effectiveResolved);
        ApplyHandleGroupPresentation(
#if MFX_PLATFORM_MACOS
            entry.handle,
#else
            &entry,
#endif
            ResolveGroupPresentation(activeManifestPath, resolved.batch.semantics.groupId));
        entry.blendMode = resolved.batch.semantics.blendMode;
        entry.sortKey = resolved.batch.semantics.sortKey;
        entry.useGroupLocalOrigin = resolved.useGroupLocalOrigin;
        entry.sourceResolved = resolved;
        {
            const GroupLayerState layerState = ResolveGroupLayer(activeManifestPath, resolved.batch.semantics.groupId);
            ApplyHandleGroupLayer(
#if MFX_PLATFORM_MACOS
                entry.handle,
#else
                &entry,
#endif
                ResolveEffectiveGroupBlendMode(entry.blendMode, layerState),
                ResolveEffectiveGroupSortKey(entry.sortKey, layerState));
        }
        entry.clipRect = resolved.batch.semantics.clipRect;
        entry.baseFrameLeftPx = effectiveResolved.batch.frameLeftPx;
        entry.baseFrameTopPx = effectiveResolved.batch.frameTopPx;
#if MFX_PLATFORM_WINDOWS
        entry.baseCenterX = effectiveResolved.batch.centerScreenPt.x;
        entry.baseCenterY = effectiveResolved.batch.centerScreenPt.y;
#endif
        const auto groupClipState = ResolveGroupClipRectState(activeManifestPath, resolved.batch.semantics.groupId);
        ApplyHandleGroupClipRect(
#if MFX_PLATFORM_MACOS
            entry.handle,
#else
            &entry,
#endif
            IntersectRenderClipRects(
                entry.clipRect,
                groupClipState.clipRect),
            groupClipState.maskShapeKind,
            groupClipState.cornerRadiusPx);
        {
            const GroupEffectiveOffset effectiveOffset = ResolveEntryGroupOffset(activeManifestPath, entry);
            ApplyHandleGroupTransform(
#if MFX_PLATFORM_MACOS
                entry.handle,
#else
                &entry,
#endif
                effectiveOffset.offsetXPx,
                effectiveOffset.offsetYPx);
        }
        entry.groupId = resolved.batch.semantics.groupId;
        return true;
    }

#if MFX_PLATFORM_MACOS
    if (!IsHandleActive(entry.handle)) {
        ReleaseHandle(entry.handle);
        entry.handle = nullptr;
        if (!CreateHandle(&entry, effectiveResolved, outError)) {
            entries.erase(it);
            return false;
        }
    }
    {
        UpsertHandle(entry.handle, effectiveResolved);
    }
    entry.blendMode = resolved.batch.semantics.blendMode;
    entry.sortKey = resolved.batch.semantics.sortKey;
    entry.clipRect = resolved.batch.semantics.clipRect;
    entry.useGroupLocalOrigin = resolved.useGroupLocalOrigin;
    entry.sourceResolved = resolved;
    entry.baseFrameLeftPx = effectiveResolved.batch.frameLeftPx;
    entry.baseFrameTopPx = effectiveResolved.batch.frameTopPx;
    ApplyHandleGroupPresentation(
        entry.handle,
        ResolveGroupPresentation(activeManifestPath, resolved.batch.semantics.groupId));
    {
        const GroupLayerState layerState = ResolveGroupLayer(activeManifestPath, resolved.batch.semantics.groupId);
        ApplyHandleGroupLayer(
            entry.handle,
            ResolveEffectiveGroupBlendMode(entry.blendMode, layerState),
            ResolveEffectiveGroupSortKey(entry.sortKey, layerState));
    }
    const auto groupClipState = ResolveGroupClipRectState(activeManifestPath, resolved.batch.semantics.groupId);
    ApplyHandleGroupClipRect(
        entry.handle,
        IntersectRenderClipRects(
            entry.clipRect,
            groupClipState.clipRect),
        groupClipState.maskShapeKind,
        groupClipState.cornerRadiusPx);
    {
        const GroupEffectiveOffset effectiveOffset = ResolveEntryGroupOffset(activeManifestPath, entry);
        ApplyHandleGroupTransform(entry.handle, effectiveOffset.offsetXPx, effectiveOffset.offsetYPx);
    }
#elif MFX_PLATFORM_WINDOWS
    if (entry.rippleId == 0u || !entry.state || entry.squareSizePx != effectiveResolved.batch.squareSizePx) {
        ReleaseHandle(&entry);
        if (!CreateHandle(&entry, effectiveResolved, outError)) {
            entries.erase(it);
            return false;
        }
    }
    {
        UpsertHandle(&entry, effectiveResolved);
    }
    entry.blendMode = resolved.batch.semantics.blendMode;
    entry.sortKey = resolved.batch.semantics.sortKey;
    entry.clipRect = resolved.batch.semantics.clipRect;
    entry.useGroupLocalOrigin = resolved.useGroupLocalOrigin;
    entry.sourceResolved = resolved;
    entry.baseFrameLeftPx = effectiveResolved.batch.frameLeftPx;
    entry.baseFrameTopPx = effectiveResolved.batch.frameTopPx;
    entry.baseCenterX = effectiveResolved.batch.centerScreenPt.x;
    entry.baseCenterY = effectiveResolved.batch.centerScreenPt.y;
    ApplyHandleGroupPresentation(
        &entry,
        ResolveGroupPresentation(activeManifestPath, resolved.batch.semantics.groupId));
    {
        const GroupLayerState layerState = ResolveGroupLayer(activeManifestPath, resolved.batch.semantics.groupId);
        ApplyHandleGroupLayer(
            &entry,
            ResolveEffectiveGroupBlendMode(entry.blendMode, layerState),
            ResolveEffectiveGroupSortKey(entry.sortKey, layerState));
    }
    const auto groupClipState = ResolveGroupClipRectState(activeManifestPath, resolved.batch.semantics.groupId);
    ApplyHandleGroupClipRect(
        &entry,
        IntersectRenderClipRects(
            entry.clipRect,
            groupClipState.clipRect),
        groupClipState.maskShapeKind,
        groupClipState.cornerRadiusPx);
    {
        const GroupEffectiveOffset effectiveOffset = ResolveEntryGroupOffset(activeManifestPath, entry);
        ApplyHandleGroupTransform(&entry, effectiveOffset.offsetXPx, effectiveOffset.offsetYPx);
    }
#endif
    entry.groupId = resolved.batch.semantics.groupId;
    return true;
#endif
}

bool RemoveRetainedQuadField(
    const std::wstring& activeManifestPath,
    uint32_t fieldId,
    std::string* outError) {
#if !(MFX_PLATFORM_MACOS || MFX_PLATFORM_WINDOWS)
    (void)activeManifestPath;
    (void)fieldId;
    if (outError) {
        *outError = "retained quad fields are not supported on this platform";
    }
    return false;
#else
    const std::wstring key = BuildRetainedEmitterKey(activeManifestPath, fieldId);
    if (key.empty()) {
        if (outError) {
            *outError = "retained quad field requires active manifest path and field id";
        }
        return false;
    }

    RetainedQuadFieldStore::RemoveRequests().fetch_add(1, std::memory_order_relaxed);

    RetainedQuadFieldEntry removed{};
    bool found = false;
    {
        std::lock_guard<std::mutex> lock(RetainedQuadFieldStore::Mutex());
        PruneInactiveRetainedQuadFieldsLocked();
        auto& entries = RetainedQuadFieldStore::Entries();
        auto it = entries.find(key);
        if (it != entries.end()) {
            removed = std::move(it->second);
            entries.erase(it);
            found = true;
        }
    }

    if (!found) {
        return true;
    }

#if MFX_PLATFORM_MACOS
    ReleaseHandle(removed.handle);
#elif MFX_PLATFORM_WINDOWS
    ReleaseHandle(&removed);
#endif
    return true;
#endif
}

uint32_t RemoveRetainedQuadFieldsByGroup(
    const std::wstring& activeManifestPath,
    uint32_t groupId) {
#if !(MFX_PLATFORM_MACOS || MFX_PLATFORM_WINDOWS)
    (void)activeManifestPath;
    (void)groupId;
    return 0u;
#else
    return RemoveRetainedEmittersForGroup<RetainedQuadFieldStore>(
        activeManifestPath,
        groupId,
        [](const RetainedQuadFieldEntry& entry, uint32_t candidateGroupId) {
            return entry.groupId == candidateGroupId;
        },
        [](RetainedQuadFieldEntry* entry) {
#if MFX_PLATFORM_MACOS
            ReleaseHandle(entry ? entry->handle : nullptr);
#elif MFX_PLATFORM_WINDOWS
            ReleaseHandle(entry);
#else
            (void)entry;
#endif
        });
#endif
}

void ApplyRetainedQuadFieldGroupPresentation(
    const std::wstring& activeManifestPath,
    uint32_t groupId,
    float alphaMultiplier,
    bool visible) {
#if !(MFX_PLATFORM_MACOS || MFX_PLATFORM_WINDOWS)
    (void)activeManifestPath;
    (void)groupId;
    (void)alphaMultiplier;
    (void)visible;
    return;
#else
    if (activeManifestPath.empty() || groupId == 0u) {
        return;
    }

    const GroupPresentationState presentation{
        std::clamp(alphaMultiplier, 0.0f, 1.0f),
        visible,
    };
    std::lock_guard<std::mutex> lock(RetainedQuadFieldStore::Mutex());
    PruneInactiveRetainedQuadFieldsLocked();
    for (auto& [key, entry] : RetainedQuadFieldStore::Entries()) {
        if (key.rfind(activeManifestPath + L"#", 0) != 0 || entry.groupId != groupId) {
            continue;
        }
#if MFX_PLATFORM_MACOS
        ApplyHandleGroupPresentation(entry.handle, presentation);
#elif MFX_PLATFORM_WINDOWS
        ApplyHandleGroupPresentation(&entry, presentation);
#endif
    }
#endif
}

void ApplyRetainedQuadFieldGroupClipRect(
    const std::wstring& activeManifestPath,
    uint32_t groupId,
    const mousefx::RenderClipRect& groupClipRect) {
#if !(MFX_PLATFORM_MACOS || MFX_PLATFORM_WINDOWS)
    (void)activeManifestPath;
    (void)groupId;
    (void)groupClipRect;
    return;
#else
    if (activeManifestPath.empty() || groupId == 0u) {
        return;
    }

    std::lock_guard<std::mutex> lock(RetainedQuadFieldStore::Mutex());
    PruneInactiveRetainedQuadFieldsLocked();
    for (auto& [key, entry] : RetainedQuadFieldStore::Entries()) {
        if (key.rfind(activeManifestPath + L"#", 0) != 0 || entry.groupId != groupId) {
            continue;
        }
        const auto groupClipState = ResolveGroupClipRectState(activeManifestPath, groupId);
#if MFX_PLATFORM_MACOS
        ApplyHandleGroupClipRect(
            entry.handle,
            IntersectRenderClipRects(entry.clipRect, groupClipState.clipRect),
            groupClipState.maskShapeKind,
            groupClipState.cornerRadiusPx);
#elif MFX_PLATFORM_WINDOWS
        ApplyHandleGroupClipRect(
            &entry,
            IntersectRenderClipRects(entry.clipRect, groupClipState.clipRect),
            groupClipState.maskShapeKind,
            groupClipState.cornerRadiusPx);
#endif
    }
#endif
}

void ApplyRetainedQuadFieldGroupLayer(
    const std::wstring& activeManifestPath,
    uint32_t groupId,
    bool hasBlendOverride,
    mousefx::RenderBlendMode blendMode,
    int32_t sortBias) {
#if !(MFX_PLATFORM_MACOS || MFX_PLATFORM_WINDOWS)
    (void)activeManifestPath;
    (void)groupId;
    (void)hasBlendOverride;
    (void)blendMode;
    (void)sortBias;
    return;
#else
    if (activeManifestPath.empty() || groupId == 0u) {
        return;
    }

    const GroupLayerState layerState{hasBlendOverride, blendMode, sortBias};
    std::lock_guard<std::mutex> lock(RetainedQuadFieldStore::Mutex());
    PruneInactiveRetainedQuadFieldsLocked();
    for (auto& [key, entry] : RetainedQuadFieldStore::Entries()) {
        if (key.rfind(activeManifestPath + L"#", 0) != 0 || entry.groupId != groupId) {
            continue;
        }
#if MFX_PLATFORM_MACOS
        ApplyHandleGroupLayer(
            entry.handle,
            ResolveEffectiveGroupBlendMode(entry.blendMode, layerState),
            ResolveEffectiveGroupSortKey(entry.sortKey, layerState));
#elif MFX_PLATFORM_WINDOWS
        ApplyHandleGroupLayer(
            &entry,
            ResolveEffectiveGroupBlendMode(entry.blendMode, layerState),
            ResolveEffectiveGroupSortKey(entry.sortKey, layerState));
#endif
    }
#endif
}

void ApplyRetainedQuadFieldGroupTransform(
    const std::wstring& activeManifestPath,
    uint32_t groupId,
    float offsetXPx,
    float offsetYPx,
    float rotationRad,
    float uniformScale) {
#if !(MFX_PLATFORM_MACOS || MFX_PLATFORM_WINDOWS)
    (void)activeManifestPath;
    (void)groupId;
    (void)offsetXPx;
    (void)offsetYPx;
    (void)rotationRad;
    (void)uniformScale;
    return;
#else
    if (activeManifestPath.empty() || groupId == 0u) {
        return;
    }

    std::lock_guard<std::mutex> lock(RetainedQuadFieldStore::Mutex());
    PruneInactiveRetainedQuadFieldsLocked();
    for (auto& [key, entry] : RetainedQuadFieldStore::Entries()) {
        if (key.rfind(activeManifestPath + L"#", 0) != 0 || entry.groupId != groupId) {
            continue;
        }
        (void)offsetXPx;
        (void)offsetYPx;
        (void)rotationRad;
        (void)uniformScale;
        const GroupEffectiveTransform effectiveTransform = ResolveEffectiveGroupTransform(
            activeManifestPath,
            entry.groupId,
            entry.useGroupLocalOrigin);
        const ResolvedQuadFieldCommand effectiveResolved = ResolveEntryQuadFieldCommand(activeManifestPath, entry);
#if MFX_PLATFORM_MACOS
        UpsertHandle(entry.handle, effectiveResolved);
        ApplyHandleGroupTransform(entry.handle, effectiveTransform.offsetXPx, effectiveTransform.offsetYPx);
#elif MFX_PLATFORM_WINDOWS
        UpsertHandle(&entry, effectiveResolved);
        ApplyHandleGroupTransform(&entry, effectiveTransform.offsetXPx, effectiveTransform.offsetYPx);
#endif
    }
#endif
}

void ApplyRetainedQuadFieldGroupLocalOrigin(
    const std::wstring& activeManifestPath,
    uint32_t groupId,
    float originXPx,
    float originYPx) {
#if !(MFX_PLATFORM_MACOS || MFX_PLATFORM_WINDOWS)
    (void)activeManifestPath;
    (void)groupId;
    (void)originXPx;
    (void)originYPx;
    return;
#else
    if (activeManifestPath.empty() || groupId == 0u) {
        return;
    }
    (void)originXPx;
    (void)originYPx;

    std::lock_guard<std::mutex> lock(RetainedQuadFieldStore::Mutex());
    PruneInactiveRetainedQuadFieldsLocked();
    for (auto& [key, entry] : RetainedQuadFieldStore::Entries()) {
        if (key.rfind(activeManifestPath + L"#", 0) != 0 || entry.groupId != groupId) {
            continue;
        }
        const GroupEffectiveOffset effectiveOffset = ResolveEntryGroupOffset(activeManifestPath, entry);
#if MFX_PLATFORM_MACOS
        ApplyHandleGroupTransform(entry.handle, effectiveOffset.offsetXPx, effectiveOffset.offsetYPx);
#elif MFX_PLATFORM_WINDOWS
        ApplyHandleGroupTransform(&entry, effectiveOffset.offsetXPx, effectiveOffset.offsetYPx);
#endif
    }
#endif
}

void ApplyRetainedQuadFieldGroupMaterial(
    const std::wstring& activeManifestPath,
    uint32_t groupId,
    bool hasTintOverride,
    uint32_t tintArgb,
    float intensityMultiplier,
    uint8_t styleKind,
    float styleAmount,
    float diffusionAmount,
    float persistenceAmount,
    float echoAmount,
    float echoDriftPx,
    uint8_t feedbackMode,
    float feedbackPhaseRad,
    uint8_t feedbackLayerCount,
    float feedbackLayerFalloff) {
#if !(MFX_PLATFORM_MACOS || MFX_PLATFORM_WINDOWS)
    (void)activeManifestPath;
    (void)groupId;
    (void)hasTintOverride;
    (void)tintArgb;
    (void)intensityMultiplier;
    (void)styleKind;
    (void)styleAmount;
    (void)diffusionAmount;
    (void)persistenceAmount;
    (void)echoAmount;
    (void)echoDriftPx;
    (void)feedbackMode;
    (void)feedbackPhaseRad;
    (void)feedbackLayerCount;
    (void)feedbackLayerFalloff;
    return;
#else
    if (activeManifestPath.empty() || groupId == 0u) {
        return;
    }
    (void)hasTintOverride;
    (void)tintArgb;
    (void)intensityMultiplier;
    (void)styleKind;
    (void)styleAmount;
    (void)diffusionAmount;
    (void)persistenceAmount;
    (void)echoAmount;
    (void)echoDriftPx;
    (void)feedbackMode;
    (void)feedbackPhaseRad;

    std::lock_guard<std::mutex> lock(RetainedQuadFieldStore::Mutex());
    PruneInactiveRetainedQuadFieldsLocked();
    for (auto& [key, entry] : RetainedQuadFieldStore::Entries()) {
        if (key.rfind(activeManifestPath + L"#", 0) != 0 || entry.groupId != groupId) {
            continue;
        }
        const ResolvedQuadFieldCommand effectiveResolved = ResolveEntryQuadFieldCommand(activeManifestPath, entry);
        const GroupLayerState layerState = ResolveGroupLayer(activeManifestPath, groupId);
        const auto groupClipState = ResolveGroupClipRectState(activeManifestPath, groupId);
        const GroupEffectiveOffset effectiveOffset = ResolveEntryGroupOffset(activeManifestPath, entry);
#if MFX_PLATFORM_MACOS
        UpsertHandle(entry.handle, effectiveResolved);
        ApplyHandleGroupLayer(
            entry.handle,
            ResolveEffectiveGroupBlendMode(entry.blendMode, layerState),
            ResolveEffectiveGroupSortKey(entry.sortKey, layerState));
        ApplyHandleGroupClipRect(
            entry.handle,
            IntersectRenderClipRects(entry.clipRect, groupClipState.clipRect),
            groupClipState.maskShapeKind,
            groupClipState.cornerRadiusPx);
        ApplyHandleGroupTransform(entry.handle, effectiveOffset.offsetXPx, effectiveOffset.offsetYPx);
#elif MFX_PLATFORM_WINDOWS
        UpsertHandle(&entry, effectiveResolved);
        ApplyHandleGroupLayer(
            &entry,
            ResolveEffectiveGroupBlendMode(entry.blendMode, layerState),
            ResolveEffectiveGroupSortKey(entry.sortKey, layerState));
        ApplyHandleGroupClipRect(
            &entry,
            IntersectRenderClipRects(entry.clipRect, groupClipState.clipRect),
            groupClipState.maskShapeKind,
            groupClipState.cornerRadiusPx);
        ApplyHandleGroupTransform(&entry, effectiveOffset.offsetXPx, effectiveOffset.offsetYPx);
#endif
    }
#endif
}

void ApplyRetainedQuadFieldGroupPass(
    const std::wstring& activeManifestPath,
    uint32_t groupId,
    uint8_t passKind,
    float passAmount,
    float responseAmount,
    uint8_t passMode,
    float phaseRad,
    uint8_t feedbackLayerCount,
    float feedbackLayerFalloff) {
#if !(MFX_PLATFORM_MACOS || MFX_PLATFORM_WINDOWS)
    (void)activeManifestPath;
    (void)groupId;
    (void)passKind;
    (void)passAmount;
    (void)responseAmount;
    (void)passMode;
    (void)phaseRad;
    (void)feedbackLayerCount;
    (void)feedbackLayerFalloff;
    return;
#else
    if (activeManifestPath.empty() || groupId == 0u) {
        return;
    }
    (void)passKind;
    (void)passAmount;
    (void)responseAmount;
    (void)passMode;
    (void)phaseRad;
    (void)feedbackLayerCount;
    (void)feedbackLayerFalloff;

    std::lock_guard<std::mutex> lock(RetainedQuadFieldStore::Mutex());
    PruneInactiveRetainedQuadFieldsLocked();
    for (auto& [key, entry] : RetainedQuadFieldStore::Entries()) {
        if (key.rfind(activeManifestPath + L"#", 0) != 0 || entry.groupId != groupId) {
            continue;
        }
        const ResolvedQuadFieldCommand effectiveResolved = ResolveEntryQuadFieldCommand(activeManifestPath, entry);
        const GroupLayerState layerState = ResolveGroupLayer(activeManifestPath, groupId);
        const auto groupClipState = ResolveGroupClipRectState(activeManifestPath, groupId);
        const GroupEffectiveOffset effectiveOffset = ResolveEntryGroupOffset(activeManifestPath, entry);
#if MFX_PLATFORM_MACOS
        UpsertHandle(entry.handle, effectiveResolved);
        ApplyHandleGroupLayer(
            entry.handle,
            ResolveEffectiveGroupBlendMode(entry.blendMode, layerState),
            ResolveEffectiveGroupSortKey(entry.sortKey, layerState));
        ApplyHandleGroupClipRect(
            entry.handle,
            IntersectRenderClipRects(entry.clipRect, groupClipState.clipRect),
            groupClipState.maskShapeKind,
            groupClipState.cornerRadiusPx);
        ApplyHandleGroupTransform(entry.handle, effectiveOffset.offsetXPx, effectiveOffset.offsetYPx);
#elif MFX_PLATFORM_WINDOWS
        UpsertHandle(&entry, effectiveResolved);
        ApplyHandleGroupLayer(
            &entry,
            ResolveEffectiveGroupBlendMode(entry.blendMode, layerState),
            ResolveEffectiveGroupSortKey(entry.sortKey, layerState));
        ApplyHandleGroupClipRect(
            &entry,
            IntersectRenderClipRects(entry.clipRect, groupClipState.clipRect),
            groupClipState.maskShapeKind,
            groupClipState.cornerRadiusPx);
        ApplyHandleGroupTransform(&entry, effectiveOffset.offsetXPx, effectiveOffset.offsetYPx);
#endif
    }
#endif
}

void ResetRetainedQuadFieldsForManifest(const std::wstring& activeManifestPath) {
#if MFX_PLATFORM_MACOS
    ResetRetainedEmittersForManifest<RetainedQuadFieldStore>(activeManifestPath, [](RetainedQuadFieldEntry* entry) {
        ReleaseHandle(entry ? entry->handle : nullptr);
    });
#elif MFX_PLATFORM_WINDOWS
    ResetRetainedEmittersForManifest<RetainedQuadFieldStore>(activeManifestPath, [](RetainedQuadFieldEntry* entry) {
        ReleaseHandle(entry);
    });
#else
    (void)activeManifestPath;
#endif
}

void ResetAllRetainedQuadFields() {
#if MFX_PLATFORM_MACOS
    ResetAllRetainedEmitters<RetainedQuadFieldStore>([](RetainedQuadFieldEntry* entry) {
        ReleaseHandle(entry ? entry->handle : nullptr);
    });
#elif MFX_PLATFORM_WINDOWS
    ResetAllRetainedEmitters<RetainedQuadFieldStore>([](RetainedQuadFieldEntry* entry) {
        ReleaseHandle(entry);
    });
#endif
}

RetainedQuadFieldRuntimeCounters GetRetainedQuadFieldRuntimeCounters() {
    RetainedQuadFieldRuntimeCounters counters{};
    counters.upsertRequests = RetainedQuadFieldStore::UpsertRequests().load(std::memory_order_relaxed);
    counters.removeRequests = RetainedQuadFieldStore::RemoveRequests().load(std::memory_order_relaxed);
#if MFX_PLATFORM_MACOS
    counters.activeFields = CountActiveRetainedEmitters<RetainedQuadFieldStore>(
        [](const RetainedQuadFieldEntry& entry) { return IsHandleActive(entry.handle); },
        [](RetainedQuadFieldEntry* entry) { ReleaseHandle(entry ? entry->handle : nullptr); });
#elif MFX_PLATFORM_WINDOWS
    counters.activeFields = CountActiveRetainedEmitters<RetainedQuadFieldStore>(
        [](const RetainedQuadFieldEntry& entry) { return IsHandleActive(entry); },
        [](RetainedQuadFieldEntry* entry) { ReleaseHandle(entry); });
#endif
    return counters;
}

} // namespace mousefx::wasm
