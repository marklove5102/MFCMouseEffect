#include "pch.h"

#include "MouseFx/Core/Wasm/WasmRetainedRibbonTrailRuntime.h"
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
#include "MouseFx/Core/Wasm/WasmPathGraphicsPath.h"
#include "MouseFx/Interfaces/IRippleRenderer.h"
#include "MouseFx/Renderers/RenderUtils.h"
#include "MouseFx/Styles/RippleStyle.h"
#include "MouseFx/Utils/TimeUtils.h"
#endif

#if MFX_PLATFORM_MACOS
#include "Platform/macos/Wasm/MacosWasmRetainedRibbonTrailSwiftBridge.h"
#endif

#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>
#include <vector>

namespace mousefx::wasm {

namespace {

struct RetainedTrailEntry final {
#if MFX_PLATFORM_MACOS
    void* handle = nullptr;
#elif MFX_PLATFORM_WINDOWS
    uint64_t rippleId = 0u;
    int squareSizePx = 0;
    std::shared_ptr<class WindowsRetainedRibbonTrailSharedState> state{};
#endif
    uint32_t groupId = 0u;
    mousefx::RenderBlendMode blendMode = mousefx::RenderBlendMode::Normal;
    int32_t sortKey = 0;
    mousefx::RenderClipRect clipRect{};
    bool useGroupLocalOrigin = false;
    ResolvedRibbonTrailCommand sourceResolved{};
    int32_t baseFrameLeftPx = 0;
    int32_t baseFrameTopPx = 0;
#if MFX_PLATFORM_WINDOWS
    int32_t baseCenterX = 0;
    int32_t baseCenterY = 0;
#endif
};

using RetainedTrailStore = RetainedEmitterRuntimeStore<RetainedTrailEntry>;

ResolvedRibbonTrailCommand ResolveRibbonTrailForGroupTransform(
    const ResolvedRibbonTrailCommand& source,
    const GroupEffectiveTransform& transform) {
    if (!HasGeometryGroupTransform(source.useGroupLocalOrigin, transform.rotationRad, transform.scaleX, transform.scaleY) ||
        source.sourcePoints.size() < 2u) {
        return source;
    }

    ResolvedRibbonTrailCommand resolved = source;
    const float scale = AverageGroupTransformScale(transform.scaleX, transform.scaleY);
    const float glowWidthPx = std::clamp(source.pathFill.glowWidthPx * scale, 0.0f, 64.0f);

    std::vector<ResolvedRibbonPointInput> points{};
    points.reserve(source.sourcePoints.size());
    for (const ResolvedRibbonTrailSourcePoint& point : source.sourcePoints) {
        const GroupTransformVector transformedPoint = ApplyGroupTransformToPoint(
            point.x,
            point.y,
            transform.rotationRad,
            transform.scaleX,
            transform.scaleY,
            transform.pivotXPx,
            transform.pivotYPx);
        points.push_back(ResolvedRibbonPointInput{
            ClampPathPoint(
                transformedPoint.x,
                transformedPoint.y),
            ClampPathCommandFloat(point.widthPx * scale, 12.0f, 1.0f, 240.0f),
        });
    }

    ResolvedSpawnPathFillCommand pathFill{};
    std::string error;
    if (!TryResolveRibbonPathFillGeometry(
            points,
            source.closed,
            source.pathFill.alpha,
            glowWidthPx,
            0u,
            source.ttlMs,
            source.pathFill.fillColorArgb,
            source.pathFill.glowColorArgb,
            source.pathFill.semantics,
            &pathFill,
            &error)) {
        return source;
    }

    resolved.pathFill = std::move(pathFill);
    return resolved;
}

ResolvedRibbonTrailCommand ResolveRibbonTrailForGroupMaterial(
    const ResolvedRibbonTrailCommand& source,
    const GroupMaterialState& materialState) {
    ResolvedRibbonTrailCommand resolved = source;
    const GroupMaterialStyleProfile styleProfile = ResolveGroupMaterialStyleProfile(materialState);
    resolved.pathFill.fillColorArgb = ApplyGroupMaterialToArgb(source.pathFill.fillColorArgb, materialState);
    resolved.pathFill.glowColorArgb = ApplyGroupMaterialToArgb(source.pathFill.glowColorArgb, materialState);
    resolved.pathFill.alpha = ClampPathCommandFloat(
        source.pathFill.alpha * styleProfile.alphaMultiplier,
        source.pathFill.alpha,
        0.0f,
        1.0f);
    resolved.pathFill.glowWidthPx = ClampPathCommandFloat(
        source.pathFill.glowWidthPx * styleProfile.glowWidthMultiplier,
        source.pathFill.glowWidthPx,
        0.0f,
        64.0f);
    resolved.ttlMs = std::clamp<uint32_t>(
        static_cast<uint32_t>(std::lround(static_cast<double>(source.ttlMs) * styleProfile.ttlMultiplier)),
        40u,
        15000u);
    const float motionX = source.sourcePoints.size() >= 2
        ? source.sourcePoints.back().x - source.sourcePoints.front().x
        : 0.0f;
    const float motionY = source.sourcePoints.size() >= 2
        ? source.sourcePoints.back().y - source.sourcePoints.front().y
        : 0.0f;
    const GroupMaterialEchoVector echoDrift = ResolveGroupMaterialEchoDrift(
        motionX,
        motionY,
        0.0f,
        styleProfile.echoDriftPx,
        styleProfile.feedbackMode,
        styleProfile.feedbackPhaseRad,
        styleProfile.feedbackLayerCount,
        styleProfile.feedbackLayerFalloff);
    const size_t pointCount = resolved.sourcePoints.size();
    for (size_t index = 0; index < pointCount; ++index) {
        auto& point = resolved.sourcePoints[index];
        point.widthPx = ClampPathCommandFloat(
            point.widthPx * styleProfile.sizeMultiplier,
            point.widthPx,
            1.0f,
            240.0f);
        const float t = pointCount > 1 ? static_cast<float>(index) / static_cast<float>(pointCount - 1) : 1.0f;
        point.x += echoDrift.x * t;
        point.y += echoDrift.y * t;
    }
    return resolved;
}

ResolvedRibbonTrailCommand ResolveRibbonTrailForGroupPass(
    const ResolvedRibbonTrailCommand& source,
    const GroupPassState& passState) {
    ResolvedRibbonTrailCommand resolved = source;
    const GroupPassStyleProfile passProfile = ResolveGroupPassStyleProfileForLane(
        passState,
        kGroupPassRouteRibbon);
    const float motionX = source.sourcePoints.size() >= 2
        ? source.sourcePoints.back().x - source.sourcePoints.front().x
        : 0.0f;
    const float motionY = source.sourcePoints.size() >= 2
        ? source.sourcePoints.back().y - source.sourcePoints.front().y
        : 0.0f;
    const GroupPassEchoVector echoDrift = ResolveGroupPassEchoDrift(
        motionX,
        motionY,
        0.0f,
        passProfile.echoDriftPx,
        passProfile.passMode,
        passProfile.phaseRad,
        passProfile.feedbackLayerCount,
        passProfile.feedbackLayerFalloff);
    const size_t pointCount = resolved.sourcePoints.size();
    for (size_t index = 0; index < pointCount; ++index) {
        auto& point = resolved.sourcePoints[index];
        point.widthPx = ClampPathCommandFloat(
            point.widthPx * passProfile.sizeMultiplier,
            point.widthPx,
            1.0f,
            240.0f);
        const float t = pointCount > 1 ? static_cast<float>(index) / static_cast<float>(pointCount - 1) : 1.0f;
        point.x += echoDrift.x * t;
        point.y += echoDrift.y * t;
    }
    resolved.ttlMs = std::clamp<uint32_t>(
        static_cast<uint32_t>(std::lround(static_cast<double>(source.ttlMs) * passProfile.ttlMultiplier)),
        40u,
        15000u);
    resolved.pathFill.alpha = std::clamp(source.pathFill.alpha * passProfile.alphaMultiplier, 0.0f, 1.0f);
    resolved.pathFill.glowWidthPx = ClampPathCommandFloat(
        source.pathFill.glowWidthPx * passProfile.glowWidthMultiplier,
        source.pathFill.glowWidthPx,
        0.0f,
        220.0f);
    return resolved;
}

ResolvedRibbonTrailCommand ResolveEntryRibbonTrailCommand(
    const std::wstring& activeManifestPath,
    const RetainedTrailEntry& entry) {
    return ResolveRibbonTrailForGroupTransform(
        ResolveRibbonTrailForGroupPass(
            ResolveRibbonTrailForGroupMaterial(
                entry.sourceResolved,
                ResolveGroupMaterial(activeManifestPath, entry.groupId)),
            ResolveGroupPass(activeManifestPath, entry.groupId)),
        ResolveEffectiveGroupTransform(activeManifestPath, entry.groupId, entry.useGroupLocalOrigin));
}

#if MFX_PLATFORM_MACOS
bool IsHandleActive(void* handle) {
    return handle != nullptr && mfx_macos_wasm_retained_ribbon_trail_is_active_v1(handle) != 0;
}

void ReleaseHandle(void* handle) {
    if (handle != nullptr) {
        mfx_macos_wasm_retained_ribbon_trail_release_v1(handle);
    }
}

void UpsertHandle(void* handle, const ResolvedRibbonTrailCommand& resolved) {
    mfx_macos_wasm_retained_ribbon_trail_upsert_v1(
        handle,
        resolved.pathFill.frameLeftPx,
        resolved.pathFill.frameTopPx,
        resolved.pathFill.squareSizePx,
        resolved.pathFill.localNodes.data(),
        static_cast<uint32_t>(resolved.pathFill.localNodes.size()),
        resolved.pathFill.alpha,
        resolved.pathFill.glowWidthPx,
        resolved.pathFill.fillColorArgb,
        resolved.pathFill.glowColorArgb,
        resolved.ttlMs,
        static_cast<uint32_t>(resolved.pathFill.semantics.blendMode),
        resolved.pathFill.semantics.sortKey,
        resolved.pathFill.semantics.groupId,
        resolved.pathFill.semantics.clipRect.leftPx,
        resolved.pathFill.semantics.clipRect.topPx,
        resolved.pathFill.semantics.clipRect.widthPx,
        resolved.pathFill.semantics.clipRect.heightPx);
}

void ApplyHandleGroupPresentation(void* handle, const GroupPresentationState& presentation) {
    if (handle == nullptr) {
        return;
    }
    mfx_macos_wasm_retained_ribbon_trail_set_group_presentation_v1(
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
    mfx_macos_wasm_retained_ribbon_trail_set_effective_clip_rect_v2(
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
    mfx_macos_wasm_retained_ribbon_trail_set_effective_layer_v1(
        handle,
        static_cast<uint32_t>(blendMode),
        sortKey);
}

void ApplyHandleGroupTransform(void* handle, float offsetXPx, float offsetYPx) {
    if (handle == nullptr) {
        return;
    }
    mfx_macos_wasm_retained_ribbon_trail_set_effective_translation_v1(handle, offsetXPx, offsetYPx);
}
#elif MFX_PLATFORM_WINDOWS
class WindowsRetainedRibbonTrailSharedState final {
public:
    void Upsert(const ResolvedRibbonTrailCommand& resolved) {
        std::lock_guard<std::mutex> lock(mutex_);
        config_ = resolved;
        alive_ = true;
        expireTickMs_ = NowMs() + static_cast<uint64_t>(resolved.ttlMs);
    }

    void RequestStop() {
        std::lock_guard<std::mutex> lock(mutex_);
        alive_ = false;
        expireTickMs_ = 0u;
    }

    bool IsAlive() const {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!alive_) {
            return false;
        }
        return NowMs() < expireTickMs_;
    }

    void Render(Gdiplus::Graphics& g) {
        ResolvedRibbonTrailCommand config{};
        uint64_t expireTickMs = 0u;
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
            config = config_;
            expireTickMs = expireTickMs_;
        }

        Gdiplus::GraphicsPath path;
        if (!BuildPathGraphicsPath(config.pathFill.localNodes, ResolvePathFillMode(config.pathFill.fillRule), &path)) {
            return;
        }

        const uint64_t nowMs = NowMs();
        const float progress = 1.0f - static_cast<float>(std::max<uint64_t>(0u, expireTickMs - nowMs)) /
            static_cast<float>(std::max<uint32_t>(1u, config.ttlMs));
        const float fade = std::max(0.0f, 1.0f - progress * progress);
        const Gdiplus::Color fillBase = render_utils::ToGdiPlus({config.pathFill.fillColorArgb});
        const Gdiplus::Color glowBase = render_utils::ToGdiPlus({config.pathFill.glowColorArgb});

        g.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
        if (config.pathFill.glowWidthPx > 0.0f) {
            for (int glowPass = 0; glowPass < 3; ++glowPass) {
                const float width = config.pathFill.glowWidthPx + static_cast<float>(glowPass) * 5.0f;
                const BYTE glowAlpha = render_utils::ClampByte(static_cast<int>(
                    static_cast<float>(glowBase.GetA()) * fade * (0.30f - static_cast<float>(glowPass) * 0.07f)));
                Gdiplus::Pen glowPen(
                    Gdiplus::Color(glowAlpha, glowBase.GetR(), glowBase.GetG(), glowBase.GetB()),
                    std::max(1.0f, width));
                glowPen.SetStartCap(Gdiplus::LineCapRound);
                glowPen.SetEndCap(Gdiplus::LineCapRound);
                glowPen.SetLineJoin(Gdiplus::LineJoinRound);
                g.DrawPath(&glowPen, &path);
            }
        }

        const BYTE fillAlpha = render_utils::ClampByte(static_cast<int>(static_cast<float>(fillBase.GetA()) * fade));
        Gdiplus::SolidBrush fillBrush(Gdiplus::Color(fillAlpha, fillBase.GetR(), fillBase.GetG(), fillBase.GetB()));
        g.FillPath(&fillBrush, &path);
    }

private:
    mutable std::mutex mutex_{};
    ResolvedRibbonTrailCommand config_{};
    bool alive_ = false;
    uint64_t expireTickMs_ = 0u;
};

class WindowsRetainedRibbonTrailRenderer final : public IRippleRenderer {
public:
    explicit WindowsRetainedRibbonTrailRenderer(std::shared_ptr<WindowsRetainedRibbonTrailSharedState> state)
        : state_(std::move(state)) {}

    void Render(Gdiplus::Graphics& g, float, uint64_t, int, const RippleStyle&) override {
        if (state_) {
            state_->Render(g);
        }
    }

    bool IsAlive() const override {
        return state_ && state_->IsAlive();
    }

private:
    std::shared_ptr<WindowsRetainedRibbonTrailSharedState> state_{};
};

bool IsHandleActive(const RetainedTrailEntry& entry) {
    if (entry.rippleId == 0u || !entry.state) {
        return false;
    }
    if (!entry.state->IsAlive()) {
        OverlayHostService::Instance().StopRipple(entry.rippleId);
        return false;
    }
    return OverlayHostService::Instance().IsRippleActive(entry.rippleId);
}

void ReleaseHandle(RetainedTrailEntry* entry) {
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

bool CreateHandle(
    RetainedTrailEntry* entry,
    const ResolvedRibbonTrailCommand& resolved,
    std::string* outError) {
    if (!entry) {
        if (outError) {
            *outError = "retained ribbon trail entry is null";
        }
        return false;
    }

    auto state = std::make_shared<WindowsRetainedRibbonTrailSharedState>();
    state->Upsert(resolved);

    ClickEvent ev{};
    ev.button = MouseButton::Left;
    ev.pt = resolved.pathFill.centerScreenPt;

    RippleStyle style{};
    style.durationMs = 1u;
    style.windowSize = resolved.pathFill.squareSizePx;
    style.startRadius = 0.0f;
    style.endRadius = 0.0f;
    style.strokeWidth = std::max(1.0f, resolved.pathFill.glowWidthPx);
    style.fill = {resolved.pathFill.fillArgb};
    style.stroke = {resolved.pathFill.fillArgb};
    style.glow = {resolved.pathFill.glowArgb};

    RenderParams params{};
    params.loop = false;
    params.intensity = 1.0f;
    params.semantics = resolved.pathFill.semantics;

    const uint64_t rippleId = OverlayHostService::Instance().ShowContinuousRipple(
        ev,
        style,
        std::make_unique<WindowsRetainedRibbonTrailRenderer>(state),
        params);
    if (rippleId == 0u) {
        if (outError) {
            *outError = "failed to create retained ribbon trail overlay";
        }
        return false;
    }

    entry->rippleId = rippleId;
    entry->squareSizePx = resolved.pathFill.squareSizePx;
    entry->state = std::move(state);
    entry->groupId = resolved.pathFill.semantics.groupId;
    return true;
}

void UpsertHandle(RetainedTrailEntry* entry, const ResolvedRibbonTrailCommand& resolved) {
    if (!entry || entry->rippleId == 0u || !entry->state) {
        return;
    }
    entry->state->Upsert(resolved);
    OverlayHostService::Instance().UpdateRipplePosition(entry->rippleId, resolved.pathFill.centerScreenPt);
}

void ApplyHandleGroupPresentation(RetainedTrailEntry* entry, const GroupPresentationState&) {
    (void)entry;
}

void ApplyHandleGroupClipRect(RetainedTrailEntry* entry, const mousefx::RenderClipRect&, uint8_t, float) {
    (void)entry;
}

void ApplyHandleGroupLayer(RetainedTrailEntry* entry, mousefx::RenderBlendMode, int32_t) {
    (void)entry;
}

void ApplyHandleGroupTransform(RetainedTrailEntry* entry, float offsetXPx, float offsetYPx) {
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
    const RetainedTrailEntry& entry) {
    return ResolveEffectiveGroupOffset(activeManifestPath, entry.groupId, entry.useGroupLocalOrigin);
}

void PruneInactiveRetainedTrailsLocked() {
    ::mousefx::wasm::PruneInactiveRetainedEmittersLocked<RetainedTrailStore>(
        [](const RetainedTrailEntry& entry) {
#if MFX_PLATFORM_MACOS
            return IsHandleActive(entry.handle);
#elif MFX_PLATFORM_WINDOWS
            return IsHandleActive(entry);
#else
            return true;
#endif
        },
        [](RetainedTrailEntry* entry) {
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

bool UpsertRetainedRibbonTrail(
    const std::wstring& activeManifestPath,
    const ResolvedRibbonTrailCommand& resolved,
    std::string* outError) {
#if !(MFX_PLATFORM_MACOS || MFX_PLATFORM_WINDOWS)
    (void)activeManifestPath;
    (void)resolved;
    if (outError) {
        *outError = "retained ribbon trails are not supported on this platform";
    }
    return false;
#else
    const std::wstring key = BuildRetainedEmitterKey(activeManifestPath, resolved.trailId);
    if (key.empty()) {
        if (outError) {
            *outError = "retained ribbon trail requires active manifest path and trail id";
        }
        return false;
    }

    const ResolvedRibbonTrailCommand effectiveResolved = ResolveRibbonTrailForGroupTransform(
        ResolveRibbonTrailForGroupMaterial(
            resolved,
            ResolveGroupMaterial(activeManifestPath, resolved.pathFill.semantics.groupId)),
        ResolveEffectiveGroupTransform(activeManifestPath, resolved.pathFill.semantics.groupId, resolved.useGroupLocalOrigin));

    std::lock_guard<std::mutex> lock(RetainedTrailStore::Mutex());
    PruneInactiveRetainedTrailsLocked();
    auto& entries = RetainedTrailStore::Entries();
    auto [it, inserted] = entries.try_emplace(key, RetainedTrailEntry{});
    if (inserted
#if MFX_PLATFORM_MACOS
        || it->second.handle == nullptr
#elif MFX_PLATFORM_WINDOWS
        || it->second.rippleId == 0u
        || !it->second.state
#endif
    ) {
#if MFX_PLATFORM_MACOS
        it->second.handle = mfx_macos_wasm_retained_ribbon_trail_create_v1();
        if (it->second.handle == nullptr) {
            entries.erase(it);
            if (outError) {
                *outError = "failed to create retained ribbon trail handle";
            }
            return false;
        }
#elif MFX_PLATFORM_WINDOWS
        if (!CreateHandle(&it->second, effectiveResolved, outError)) {
            entries.erase(it);
            return false;
        }
#endif
    } else {
#if MFX_PLATFORM_WINDOWS
        if (it->second.squareSizePx != effectiveResolved.pathFill.squareSizePx) {
            ReleaseHandle(&it->second);
            if (!CreateHandle(&it->second, effectiveResolved, outError)) {
                entries.erase(it);
                return false;
            }
        }
#endif
    }

#if MFX_PLATFORM_MACOS
    UpsertHandle(it->second.handle, effectiveResolved);
    it->second.groupId = resolved.pathFill.semantics.groupId;
    it->second.blendMode = resolved.pathFill.semantics.blendMode;
    it->second.sortKey = resolved.pathFill.semantics.sortKey;
    it->second.clipRect = resolved.pathFill.semantics.clipRect;
    it->second.useGroupLocalOrigin = resolved.useGroupLocalOrigin;
    it->second.sourceResolved = resolved;
    it->second.baseFrameLeftPx = effectiveResolved.pathFill.frameLeftPx;
    it->second.baseFrameTopPx = effectiveResolved.pathFill.frameTopPx;
    ApplyHandleGroupPresentation(
        it->second.handle,
        ResolveGroupPresentation(activeManifestPath, resolved.pathFill.semantics.groupId));
    {
        const GroupLayerState layerState = ResolveGroupLayer(activeManifestPath, resolved.pathFill.semantics.groupId);
        ApplyHandleGroupLayer(
            it->second.handle,
            ResolveEffectiveGroupBlendMode(it->second.blendMode, layerState),
            ResolveEffectiveGroupSortKey(it->second.sortKey, layerState));
    }
    const auto groupClipState = ResolveGroupClipRectState(activeManifestPath, resolved.pathFill.semantics.groupId);
    ApplyHandleGroupClipRect(
        it->second.handle,
        IntersectRenderClipRects(
            it->second.clipRect,
            groupClipState.clipRect),
        groupClipState.maskShapeKind,
        groupClipState.cornerRadiusPx);
    {
        const GroupEffectiveOffset effectiveOffset = ResolveEntryGroupOffset(activeManifestPath, it->second);
        ApplyHandleGroupTransform(it->second.handle, effectiveOffset.offsetXPx, effectiveOffset.offsetYPx);
    }
#elif MFX_PLATFORM_WINDOWS
    UpsertHandle(&it->second, effectiveResolved);
    it->second.groupId = resolved.pathFill.semantics.groupId;
    it->second.blendMode = resolved.pathFill.semantics.blendMode;
    it->second.sortKey = resolved.pathFill.semantics.sortKey;
    it->second.clipRect = resolved.pathFill.semantics.clipRect;
    it->second.useGroupLocalOrigin = resolved.useGroupLocalOrigin;
    it->second.sourceResolved = resolved;
    it->second.baseFrameLeftPx = effectiveResolved.pathFill.frameLeftPx;
    it->second.baseFrameTopPx = effectiveResolved.pathFill.frameTopPx;
    it->second.baseCenterX = effectiveResolved.pathFill.centerScreenPt.x;
    it->second.baseCenterY = effectiveResolved.pathFill.centerScreenPt.y;
    ApplyHandleGroupPresentation(
        &it->second,
        ResolveGroupPresentation(activeManifestPath, resolved.pathFill.semantics.groupId));
    {
        const GroupLayerState layerState = ResolveGroupLayer(activeManifestPath, resolved.pathFill.semantics.groupId);
        ApplyHandleGroupLayer(
            &it->second,
            ResolveEffectiveGroupBlendMode(it->second.blendMode, layerState),
            ResolveEffectiveGroupSortKey(it->second.sortKey, layerState));
    }
    const auto groupClipState = ResolveGroupClipRectState(activeManifestPath, resolved.pathFill.semantics.groupId);
    ApplyHandleGroupClipRect(
        &it->second,
        IntersectRenderClipRects(
            it->second.clipRect,
            groupClipState.clipRect),
        groupClipState.maskShapeKind,
        groupClipState.cornerRadiusPx);
    {
        const GroupEffectiveOffset effectiveOffset = ResolveEntryGroupOffset(activeManifestPath, it->second);
        ApplyHandleGroupTransform(&it->second, effectiveOffset.offsetXPx, effectiveOffset.offsetYPx);
    }
#endif
    RetainedTrailStore::UpsertRequests().fetch_add(1u, std::memory_order_relaxed);
    return true;
#endif
}

bool RemoveRetainedRibbonTrail(
    const std::wstring& activeManifestPath,
    uint32_t trailId,
    std::string* outError) {
#if !(MFX_PLATFORM_MACOS || MFX_PLATFORM_WINDOWS)
    (void)activeManifestPath;
    (void)trailId;
    if (outError) {
        *outError = "retained ribbon trails are not supported on this platform";
    }
    return false;
#else
    const std::wstring key = BuildRetainedEmitterKey(activeManifestPath, trailId);
    if (key.empty()) {
        if (outError) {
            *outError = "retained ribbon trail remove requires active manifest path and trail id";
        }
        return false;
    }

    void* handle = nullptr;
#if MFX_PLATFORM_WINDOWS
    RetainedTrailEntry entry{};
#endif
    {
        std::lock_guard<std::mutex> lock(RetainedTrailStore::Mutex());
        auto& entries = RetainedTrailStore::Entries();
        const auto it = entries.find(key);
        if (it != entries.end()) {
#if MFX_PLATFORM_MACOS
            handle = it->second.handle;
#elif MFX_PLATFORM_WINDOWS
            entry = std::move(it->second);
#endif
            entries.erase(it);
        }
    }

#if MFX_PLATFORM_MACOS
    ReleaseHandle(handle);
#elif MFX_PLATFORM_WINDOWS
    ReleaseHandle(&entry);
#endif
    RetainedTrailStore::RemoveRequests().fetch_add(1u, std::memory_order_relaxed);
    return true;
#endif
}

uint32_t RemoveRetainedRibbonTrailsByGroup(
    const std::wstring& activeManifestPath,
    uint32_t groupId) {
#if !(MFX_PLATFORM_MACOS || MFX_PLATFORM_WINDOWS)
    (void)activeManifestPath;
    (void)groupId;
    return 0u;
#else
    return RemoveRetainedEmittersForGroup<RetainedTrailStore>(
        activeManifestPath,
        groupId,
        [](const RetainedTrailEntry& entry, uint32_t candidateGroupId) {
            return entry.groupId == candidateGroupId;
        },
        [](RetainedTrailEntry* entry) {
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

void ApplyRetainedRibbonTrailGroupPresentation(
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
    std::lock_guard<std::mutex> lock(RetainedTrailStore::Mutex());
    PruneInactiveRetainedTrailsLocked();
    for (auto& [key, entry] : RetainedTrailStore::Entries()) {
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

void ApplyRetainedRibbonTrailGroupClipRect(
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

    std::lock_guard<std::mutex> lock(RetainedTrailStore::Mutex());
    PruneInactiveRetainedTrailsLocked();
    for (auto& [key, entry] : RetainedTrailStore::Entries()) {
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

void ApplyRetainedRibbonTrailGroupLayer(
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
    std::lock_guard<std::mutex> lock(RetainedTrailStore::Mutex());
    PruneInactiveRetainedTrailsLocked();
    for (auto& [key, entry] : RetainedTrailStore::Entries()) {
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

void ApplyRetainedRibbonTrailGroupTransform(
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

    std::lock_guard<std::mutex> lock(RetainedTrailStore::Mutex());
    PruneInactiveRetainedTrailsLocked();
    for (auto& [key, entry] : RetainedTrailStore::Entries()) {
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
        const ResolvedRibbonTrailCommand effectiveResolved = ResolveEntryRibbonTrailCommand(activeManifestPath, entry);
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

void ApplyRetainedRibbonTrailGroupLocalOrigin(
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

    std::lock_guard<std::mutex> lock(RetainedTrailStore::Mutex());
    PruneInactiveRetainedTrailsLocked();
    for (auto& [key, entry] : RetainedTrailStore::Entries()) {
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

void ApplyRetainedRibbonTrailGroupMaterial(
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

    std::lock_guard<std::mutex> lock(RetainedTrailStore::Mutex());
    PruneInactiveRetainedTrailsLocked();
    for (auto& [key, entry] : RetainedTrailStore::Entries()) {
        if (key.rfind(activeManifestPath + L"#", 0) != 0 || entry.groupId != groupId) {
            continue;
        }
        const ResolvedRibbonTrailCommand effectiveResolved = ResolveEntryRibbonTrailCommand(activeManifestPath, entry);
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

void ApplyRetainedRibbonTrailGroupPass(
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

    std::lock_guard<std::mutex> lock(RetainedTrailStore::Mutex());
    PruneInactiveRetainedTrailsLocked();
    for (auto& [key, entry] : RetainedTrailStore::Entries()) {
        if (key.rfind(activeManifestPath + L"#", 0) != 0 || entry.groupId != groupId) {
            continue;
        }
        const ResolvedRibbonTrailCommand effectiveResolved = ResolveEntryRibbonTrailCommand(activeManifestPath, entry);
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

void ResetRetainedRibbonTrailsForManifest(const std::wstring& activeManifestPath) {
#if !(MFX_PLATFORM_MACOS || MFX_PLATFORM_WINDOWS)
    (void)activeManifestPath;
    return;
#else
    if (activeManifestPath.empty()) {
        return;
    }

    ResetRetainedEmittersForManifest<RetainedTrailStore>(
        activeManifestPath,
        [](RetainedTrailEntry* entry) {
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

void ResetAllRetainedRibbonTrails() {
#if !(MFX_PLATFORM_MACOS || MFX_PLATFORM_WINDOWS)
    return;
#else
    ResetAllRetainedEmitters<RetainedTrailStore>(
        [](RetainedTrailEntry* entry) {
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

RetainedRibbonTrailRuntimeCounters GetRetainedRibbonTrailRuntimeCounters() {
    RetainedRibbonTrailRuntimeCounters counters{};
    counters.upsertRequests = RetainedTrailStore::UpsertRequests().load(std::memory_order_relaxed);
    counters.removeRequests = RetainedTrailStore::RemoveRequests().load(std::memory_order_relaxed);
#if MFX_PLATFORM_MACOS || MFX_PLATFORM_WINDOWS
    counters.activeTrails = CountActiveRetainedEmitters<RetainedTrailStore>(
        [](const RetainedTrailEntry& entry) {
#if MFX_PLATFORM_MACOS
            return IsHandleActive(entry.handle);
#elif MFX_PLATFORM_WINDOWS
            return IsHandleActive(entry);
#else
            return true;
#endif
        },
        [](RetainedTrailEntry* entry) {
#if MFX_PLATFORM_MACOS
            ReleaseHandle(entry ? entry->handle : nullptr);
#elif MFX_PLATFORM_WINDOWS
            ReleaseHandle(entry);
#else
            (void)entry;
#endif
        });
#endif
    return counters;
}

} // namespace mousefx::wasm
