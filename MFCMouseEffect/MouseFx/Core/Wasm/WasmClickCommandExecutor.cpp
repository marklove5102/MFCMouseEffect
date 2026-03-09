#include "pch.h"

#include "WasmClickCommandExecutor.h"

#include "MouseFx/Core/Overlay/OverlayHostService.h"
#include "MouseFx/Core/Wasm/WasmGlowBatchCommandConfig.h"
#include "MouseFx/Core/Wasm/WasmGroupCommandConfig.h"
#include "MouseFx/Core/Wasm/WasmGroupClipRectRuntime.h"
#include "MouseFx/Core/Wasm/WasmGroupLocalOriginRuntime.h"
#include "MouseFx/Core/Wasm/WasmGroupPassRuntime.h"
#include "MouseFx/Core/Wasm/WasmGroupMaterialRuntime.h"
#include "MouseFx/Core/Wasm/WasmGroupLayerRuntime.h"
#include "MouseFx/Core/Wasm/WasmGroupTransformRuntime.h"
#include "MouseFx/Core/Wasm/WasmGlowEmitterCommandConfig.h"
#include "MouseFx/Core/Wasm/WasmParticleEmitterCommandConfig.h"
#include "MouseFx/Core/Wasm/WasmGroupPresentationRuntime.h"
#include "MouseFx/Core/Wasm/WasmPathFillCommandExecutor.h"
#include "MouseFx/Core/Wasm/WasmPathStrokeCommandExecutor.h"
#include "MouseFx/Core/Wasm/WasmQuadFieldCommandConfig.h"
#include "MouseFx/Core/Wasm/WasmRibbonStripCommandExecutor.h"
#include "MouseFx/Core/Wasm/WasmRibbonTrailCommandConfig.h"
#include "MouseFx/Core/Wasm/WasmPolylineCommandConfig.h"
#include "MouseFx/Core/Wasm/WasmPulseCommandConfig.h"
#include "MouseFx/Core/Wasm/WasmRetainedGlowEmitterRuntime.h"
#include "MouseFx/Core/Wasm/WasmRetainedParticleEmitterRuntime.h"
#include "MouseFx/Core/Wasm/WasmRetainedQuadFieldRuntime.h"
#include "MouseFx/Core/Wasm/WasmRetainedRibbonTrailRuntime.h"
#include "MouseFx/Core/Wasm/WasmRetainedSpriteEmitterRuntime.h"
#include "MouseFx/Core/Wasm/WasmSpriteEmitterCommandConfig.h"
#include "MouseFx/Core/Wasm/WasmSpriteBatchCommandExecutor.h"
#include "WasmCommandBufferParser.h"
#include "WasmImageCommandConfig.h"
#include "WasmImageRuntimeConfig.h"
#include "WasmPluginAbi.h"
#include "WasmRenderResourceResolver.h"
#include "WasmTextCommandConfig.h"
#include "MouseFx/Renderers/RenderUtils.h"
#include "MouseFx/Renderers/Click/RippleRenderer.h"
#include "MouseFx/Renderers/Click/StarRenderer.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <utility>
#include <vector>

namespace mousefx::wasm {

namespace {

void ExecuteSpawnText(
    const SpawnTextCommandV1& cmd,
    const EffectConfig& config,
    CommandExecutionResult* outResult) {
    if (!outResult) {
        return;
    }
    const TextConfig textCfg = BuildSpawnTextConfig(config.textClick, cmd);
    const ScreenPoint pt{
        static_cast<LONG>(std::lround(cmd.x)),
        static_cast<LONG>(std::lround(cmd.y)),
    };
    const std::wstring text = WasmRenderResourceResolver::ResolveTextById(config, cmd.textId);
    const Argb color = WasmRenderResourceResolver::ResolveTextColor(config, cmd.textId, cmd.colorRgba);

    if (!OverlayHostService::Instance().ShowText(pt, text, color, textCfg)) {
        outResult->lastError = "failed to render spawn_text command";
        outResult->droppedCommands += 1;
        return;
    }
    outResult->executedTextCommands += 1;
    outResult->renderedAny = true;
}

RippleStyle BuildImageStyle(const EffectConfig& config, const SpawnImageCommandV1& cmd) {
    RippleStyle style{};
    style.durationMs = ResolveSpawnImageLifeMs(cmd.lifeMs, config.icon.durationMs);
    const float scale = ResolveSpawnImageScale(cmd.scale);
    style.startRadius = std::max(2.0f, config.icon.startRadius * scale);
    style.endRadius = std::max(style.startRadius + 2.0f, config.icon.endRadius * scale);
    style.strokeWidth = std::max(0.8f, config.icon.strokeWidth);

    const float diameter = style.endRadius * 2.0f;
    const float windowPadding = std::max(8.0f, diameter * 0.15f);
    style.windowSize = std::clamp<int>(
        static_cast<int>(std::ceil(diameter + windowPadding)),
        20,
        640);

    const Argb tint = WasmRenderResourceResolver::ResolveImageTint(config, cmd.imageId, cmd.tintRgba);
    style.fill = tint;
    style.stroke = tint;
    style.glow = Argb{(tint.value & 0x00FFFFFFu) | 0x44000000u};
    return style;
}

void ExecuteSpawnImage(
    const SpawnImageCommandV1& cmd,
    const EffectConfig& config,
    const std::wstring& activeManifestPath,
    CommandExecutionResult* outResult) {
    if (!outResult) {
        return;
    }

    ClickEvent ev{};
    ev.button = MouseButton::Left;
    ev.pt.x = static_cast<LONG>(std::lround(cmd.x));
    ev.pt.y = static_cast<LONG>(std::lround(cmd.y));

    RenderParams renderParams{};
    renderParams.loop = false;
    renderParams.intensity = ResolveSpawnImageAlpha(cmd.alpha);
    renderParams.directionRad = cmd.rotation;
    renderParams.velocityX = cmd.vx;
    renderParams.velocityY = cmd.vy;
    renderParams.accelerationX = cmd.ax;
    renderParams.accelerationY = cmd.ay;
    renderParams.useKinematics = (std::abs(cmd.vx) > 0.001f) || (std::abs(cmd.vy) > 0.001f) ||
                                 (std::abs(cmd.ax) > 0.001f) || (std::abs(cmd.ay) > 0.001f);
    renderParams.startDelayMs = ResolveSpawnImageDelayMs(cmd.delayMs);

    const RippleStyle style = BuildImageStyle(config, cmd);
    const bool applyTint = ResolveSpawnImageApplyTint(cmd.tintRgba);
    std::string rendererKey;
    std::unique_ptr<IRippleRenderer> renderer =
        WasmRenderResourceResolver::CreateImageRendererById(
            cmd.imageId,
            activeManifestPath,
            cmd.tintRgba,
            applyTint,
            renderParams.intensity,
            &rendererKey);
    if (!renderer) {
        outResult->lastError = "cannot resolve image renderer";
        outResult->droppedCommands += 1;
        return;
    }

    const uint64_t id = OverlayHostService::Instance().ShowRipple(
        ev, style, std::move(renderer), renderParams);
    if (id == 0) {
        outResult->lastError = "failed to render spawn_image command";
        outResult->droppedCommands += 1;
        return;
    }

    outResult->executedImageCommands += 1;
    outResult->renderedAny = true;
}

std::unique_ptr<IRippleRenderer> CreateSpawnPulseRenderer(const std::string& normalizedType) {
    if (normalizedType == "star") {
        return std::make_unique<StarRenderer>();
    }
    return std::make_unique<RippleRenderer>();
}

class SpawnPolylineRenderer final : public IRippleRenderer {
public:
    SpawnPolylineRenderer(
        std::vector<float> localPointsXY,
        bool closed,
        float lineWidthPx,
        uint32_t strokeArgb,
        uint32_t glowArgb)
        : localPointsXY_(std::move(localPointsXY)),
          closed_(closed),
          lineWidthPx_(lineWidthPx),
          strokeArgb_(strokeArgb),
          glowArgb_(glowArgb) {}

    void Render(
        Gdiplus::Graphics& g,
        float t,
        uint64_t,
        int,
        const RippleStyle&) override {
        if (localPointsXY_.size() < 4u) {
            return;
        }

        std::vector<Gdiplus::PointF> points;
        points.reserve(localPointsXY_.size() / 2u);
        for (size_t index = 0; index + 1 < localPointsXY_.size(); index += 2) {
            points.push_back(Gdiplus::PointF(localPointsXY_[index], localPointsXY_[index + 1]));
        }
        if (points.size() < 2u) {
            return;
        }

        const float clampedT = render_utils::Clamp01(t);
        const float eased = 1.0f - (1.0f - clampedT) * (1.0f - clampedT) * (1.0f - clampedT);
        const float alpha = 1.0f - eased;
        const Gdiplus::Color strokeBase = render_utils::ToGdiPlus({strokeArgb_});
        const Gdiplus::Color glowBase = render_utils::ToGdiPlus({glowArgb_});

        auto drawPath = [&](Gdiplus::Pen* pen) {
            if (closed_) {
                g.DrawPolygon(pen, points.data(), static_cast<INT>(points.size()));
            } else {
                g.DrawLines(pen, points.data(), static_cast<INT>(points.size()));
            }
        };

        for (int glowPass = 0; glowPass < 3; ++glowPass) {
            const float width = lineWidthPx_ + 10.0f + static_cast<float>(glowPass) * 4.0f;
            const BYTE glowAlpha = render_utils::ClampByte(static_cast<int>(
                static_cast<float>(glowBase.GetA()) * alpha * (0.34f - static_cast<float>(glowPass) * 0.08f)));
            Gdiplus::Pen glowPen(
                Gdiplus::Color(glowAlpha, glowBase.GetR(), glowBase.GetG(), glowBase.GetB()),
                width);
            glowPen.SetStartCap(Gdiplus::LineCapRound);
            glowPen.SetEndCap(Gdiplus::LineCapRound);
            glowPen.SetLineJoin(Gdiplus::LineJoinRound);
            drawPath(&glowPen);
        }

        const BYTE strokeAlpha = render_utils::ClampByte(
            static_cast<int>(static_cast<float>(strokeBase.GetA()) * alpha));
        Gdiplus::Pen strokePen(
            Gdiplus::Color(strokeAlpha, strokeBase.GetR(), strokeBase.GetG(), strokeBase.GetB()),
            lineWidthPx_);
        strokePen.SetStartCap(Gdiplus::LineCapRound);
        strokePen.SetEndCap(Gdiplus::LineCapRound);
        strokePen.SetLineJoin(Gdiplus::LineJoinRound);
        drawPath(&strokePen);
    }

private:
    std::vector<float> localPointsXY_{};
    bool closed_ = false;
    float lineWidthPx_ = 4.0f;
    uint32_t strokeArgb_ = 0xFFFFFFFFu;
    uint32_t glowArgb_ = 0x66FFFFFFu;
};

class SpawnGlowBatchRenderer final : public IRippleRenderer {
public:
    explicit SpawnGlowBatchRenderer(
        std::vector<ResolvedGlowBatchItem> items,
        bool screenBlend)
        : items_(std::move(items)),
          screenBlend_(screenBlend) {}

    void Render(
        Gdiplus::Graphics& g,
        float t,
        uint64_t elapsedMs,
        int,
        const RippleStyle&) override {
        if (items_.empty()) {
            return;
        }

        const float clampedT = render_utils::Clamp01(t);
        const float eased = 1.0f - (1.0f - clampedT) * (1.0f - clampedT) * (1.0f - clampedT);
        const float fade = std::max(0.0f, 1.0f - eased);
        const float elapsedSec = static_cast<float>(elapsedMs) / 1000.0f;
        g.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);

        for (const ResolvedGlowBatchItem& item : items_) {
            if (item.alpha <= 0.0f || item.radiusPx <= 0.0f) {
                continue;
            }

            const float x = item.localX +
                item.velocityX * elapsedSec +
                0.5f * item.accelerationX * elapsedSec * elapsedSec;
            const float y = item.localY +
                item.velocityY * elapsedSec +
                0.5f * item.accelerationY * elapsedSec * elapsedSec;
            const float radius = std::max(0.8f, item.radiusPx * (1.0f - clampedT * 0.24f));
            const Gdiplus::Color base = render_utils::ToGdiPlus({item.colorArgb});
            const float alphaScale = item.alpha * fade;
            const BYTE coreAlpha = render_utils::ClampByte(
                static_cast<int>(static_cast<float>(base.GetA()) * alphaScale));
            if (coreAlpha == 0u) {
                continue;
            }

            const float glowRadius = radius * (screenBlend_ ? 3.4f : 3.0f);
            Gdiplus::GraphicsPath glowPath;
            glowPath.AddEllipse(
                x - glowRadius,
                y - glowRadius,
                glowRadius * 2.0f,
                glowRadius * 2.0f);
            Gdiplus::PathGradientBrush glowBrush(&glowPath);
            glowBrush.SetCenterPoint(Gdiplus::PointF(x, y));
            glowBrush.SetCenterColor(Gdiplus::Color(
                render_utils::ClampByte(static_cast<int>(static_cast<float>(base.GetA()) * alphaScale * 0.34f)),
                base.GetR(),
                base.GetG(),
                base.GetB()));
            Gdiplus::Color surround[1] = {
                Gdiplus::Color(0u, base.GetR(), base.GetG(), base.GetB())
            };
            int surroundCount = 1;
            glowBrush.SetSurroundColors(surround, &surroundCount);
            g.FillPath(&glowBrush, &glowPath);

            Gdiplus::SolidBrush coreBrush(Gdiplus::Color(coreAlpha, base.GetR(), base.GetG(), base.GetB()));
            g.FillEllipse(&coreBrush, x - radius, y - radius, radius * 2.0f, radius * 2.0f);

            const float hotRadius = std::max(0.6f, radius * 0.36f);
            Gdiplus::SolidBrush hotBrush(Gdiplus::Color(
                render_utils::ClampByte(static_cast<int>(coreAlpha * 0.78f)),
                255u,
                255u,
                255u));
            g.FillEllipse(&hotBrush, x - hotRadius, y - hotRadius, hotRadius * 2.0f, hotRadius * 2.0f);
        }
    }

private:
    std::vector<ResolvedGlowBatchItem> items_{};
    bool screenBlend_ = false;
};

void ExecuteSpawnPulse(
    const SpawnPulseCommandV1& cmd,
    const EffectConfig& config,
    CommandExecutionResult* outResult) {
    if (!outResult) {
        return;
    }

    const ResolvedSpawnPulseCommand resolved = ResolveSpawnPulseCommand(config, cmd);
    ClickEvent ev{};
    ev.button = MouseButton::Left;
    ev.pt = resolved.screenPt;

    RenderParams renderParams{};
    renderParams.loop = false;
    renderParams.intensity = resolved.alpha;
    renderParams.startDelayMs = resolved.delayMs;

    std::unique_ptr<IRippleRenderer> renderer = CreateSpawnPulseRenderer(resolved.normalizedType);
    if (!renderer) {
        outResult->lastError = "cannot resolve pulse renderer";
        outResult->droppedCommands += 1;
        return;
    }

    const uint64_t id = OverlayHostService::Instance().ShowRipple(
        ev, resolved.style, std::move(renderer), renderParams);
    if (id == 0) {
        outResult->lastError = "failed to render spawn_pulse command";
        outResult->droppedCommands += 1;
        return;
    }

    outResult->executedPulseCommands += 1;
    outResult->renderedAny = true;
}

void ExecuteSpawnPolyline(
    const uint8_t* raw,
    size_t sizeBytes,
    const EffectConfig& config,
    CommandExecutionResult* outResult) {
    if (!outResult) {
        return;
    }

    ResolvedSpawnPolylineCommand resolved{};
    std::string error;
    if (!TryResolveSpawnPolylineCommand(raw, sizeBytes, config, &resolved, &error)) {
        outResult->lastError = error.empty() ? "failed to resolve spawn_polyline command" : error;
        outResult->droppedCommands += 1;
        return;
    }

    ClickEvent ev{};
    ev.button = MouseButton::Left;
    ev.pt = resolved.centerScreenPt;

    RippleStyle style{};
    style.durationMs = resolved.lifeMs;
    style.windowSize = resolved.squareSizePx;
    style.startRadius = 0.0f;
    style.endRadius = 0.0f;
    style.strokeWidth = resolved.lineWidthPx;
    style.fill = {0u};
    style.stroke = {resolved.strokeArgb};
    style.glow = {resolved.glowArgb};

    RenderParams renderParams{};
    renderParams.loop = false;
    renderParams.intensity = resolved.alpha;
    renderParams.startDelayMs = resolved.delayMs;

    auto renderer = std::make_unique<SpawnPolylineRenderer>(
        std::move(resolved.localPointsXY),
        resolved.closed,
        resolved.lineWidthPx,
        resolved.strokeArgb,
        resolved.glowArgb);
    const uint64_t id = OverlayHostService::Instance().ShowRipple(
        ev,
        style,
        std::move(renderer),
        renderParams);
    if (id == 0) {
        outResult->lastError = "failed to render spawn_polyline command";
        outResult->droppedCommands += 1;
        return;
    }

    outResult->executedPolylineCommands += 1;
    outResult->renderedAny = true;
}

void ExecuteSpawnGlowBatch(
    const uint8_t* raw,
    size_t sizeBytes,
    const EffectConfig& config,
    CommandExecutionResult* outResult) {
    if (!outResult) {
        return;
    }

    ResolvedSpawnGlowBatchCommand resolved{};
    std::string error;
    if (!TryResolveSpawnGlowBatchCommand(raw, sizeBytes, config, false, &resolved, &error)) {
        outResult->lastError = error.empty() ? "failed to resolve spawn_glow_batch command" : error;
        outResult->droppedCommands += 1;
        return;
    }

    ClickEvent ev{};
    ev.button = MouseButton::Left;
    ev.pt = resolved.centerScreenPt;

    RippleStyle style{};
    style.durationMs = resolved.lifeMs;
    style.windowSize = resolved.squareSizePx;
    style.startRadius = 0.0f;
    style.endRadius = 0.0f;
    style.strokeWidth = 0.0f;
    style.fill = {0u};
    style.stroke = {0u};
    style.glow = {0u};

    RenderParams renderParams{};
    renderParams.loop = false;
    renderParams.intensity = 1.0f;
    renderParams.startDelayMs = resolved.delayMs;
    renderParams.semantics = resolved.semantics;

    auto renderer = std::make_unique<SpawnGlowBatchRenderer>(
        std::move(resolved.items),
        UsesScreenLikeBlend(resolved.semantics.blendMode));
    const uint64_t id = OverlayHostService::Instance().ShowRipple(
        ev,
        style,
        std::move(renderer),
        renderParams);
    if (id == 0) {
        outResult->lastError = "failed to render spawn_glow_batch command";
        outResult->droppedCommands += 1;
        return;
    }

    outResult->executedGlowBatchCommands += 1;
    outResult->renderedAny = true;
}

void ExecuteUpsertGlowEmitter(
    const uint8_t* raw,
    size_t sizeBytes,
    const EffectConfig& config,
    const std::wstring& activeManifestPath,
    CommandExecutionResult* outResult) {
    if (!outResult) {
        return;
    }

    ResolvedGlowEmitterCommand resolved{};
    std::string error;
    if (!TryResolveUpsertGlowEmitterCommand(raw, sizeBytes, config, false, &resolved, &error)) {
        outResult->lastError = error.empty() ? "failed to resolve upsert_glow_emitter command" : error;
        outResult->droppedCommands += 1;
        return;
    }

    if (!UpsertRetainedGlowEmitter(activeManifestPath, resolved, &error)) {
        outResult->lastError = error.empty() ? "failed to upsert retained glow emitter" : error;
        outResult->droppedCommands += 1;
        return;
    }

    outResult->executedGlowEmitterCommands += 1;
    outResult->renderedAny = true;
}

void ExecuteRemoveGlowEmitter(
    const uint8_t* raw,
    size_t sizeBytes,
    const std::wstring& activeManifestPath,
    CommandExecutionResult* outResult) {
    if (!outResult) {
        return;
    }

    uint32_t emitterId = 0u;
    std::string error;
    if (!TryResolveRemoveGlowEmitterCommand(raw, sizeBytes, &emitterId, &error)) {
        outResult->lastError = error.empty() ? "failed to resolve remove_glow_emitter command" : error;
        outResult->droppedCommands += 1;
        return;
    }

    if (!RemoveRetainedGlowEmitter(activeManifestPath, emitterId, &error)) {
        outResult->lastError = error.empty() ? "failed to remove retained glow emitter" : error;
        outResult->droppedCommands += 1;
        return;
    }

    outResult->executedGlowEmitterRemoveCommands += 1;
    outResult->renderedAny = true;
}

void ExecuteUpsertSpriteEmitter(
    const uint8_t* raw,
    size_t sizeBytes,
    const EffectConfig& config,
    const std::wstring& activeManifestPath,
    CommandExecutionResult* outResult) {
    if (!outResult) {
        return;
    }

    ResolvedSpriteEmitterCommand resolved{};
    std::string error;
    if (!TryResolveUpsertSpriteEmitterCommand(raw, sizeBytes, config, activeManifestPath, false, &resolved, &error)) {
        outResult->lastError = std::move(error);
        outResult->droppedCommands += 1;
        return;
    }

    if (!UpsertRetainedSpriteEmitter(activeManifestPath, resolved, &error)) {
        outResult->lastError = error.empty() ? "failed to upsert retained sprite emitter" : error;
        outResult->droppedCommands += 1;
        return;
    }

    outResult->executedSpriteEmitterCommands += 1;
    outResult->renderedAny = true;
}

void ExecuteRemoveSpriteEmitter(
    const uint8_t* raw,
    size_t sizeBytes,
    const std::wstring& activeManifestPath,
    CommandExecutionResult* outResult) {
    if (!outResult) {
        return;
    }

    uint32_t emitterId = 0u;
    std::string error;
    if (!TryResolveRemoveSpriteEmitterCommand(raw, sizeBytes, &emitterId, &error)) {
        outResult->lastError = std::move(error);
        outResult->droppedCommands += 1;
        return;
    }

    if (!RemoveRetainedSpriteEmitter(activeManifestPath, emitterId, &error)) {
        outResult->lastError = error.empty() ? "failed to remove retained sprite emitter" : error;
        outResult->droppedCommands += 1;
        return;
    }

    outResult->executedSpriteEmitterRemoveCommands += 1;
    outResult->renderedAny = true;
}

void ExecuteUpsertParticleEmitter(
    const uint8_t* raw,
    size_t sizeBytes,
    const EffectConfig& config,
    const std::wstring& activeManifestPath,
    CommandExecutionResult* outResult) {
    if (!outResult) {
        return;
    }

    ResolvedParticleEmitterCommand resolved{};
    std::string error;
    if (!TryResolveUpsertParticleEmitterCommand(raw, sizeBytes, config, false, &resolved, &error)) {
        outResult->lastError = error.empty() ? "failed to resolve upsert_particle_emitter command" : error;
        outResult->droppedCommands += 1;
        return;
    }

    if (!UpsertRetainedParticleEmitter(activeManifestPath, resolved, &error)) {
        outResult->lastError = error.empty() ? "failed to upsert retained particle emitter" : error;
        outResult->droppedCommands += 1;
        return;
    }

    outResult->executedParticleEmitterCommands += 1;
    outResult->renderedAny = true;
}

void ExecuteRemoveParticleEmitter(
    const uint8_t* raw,
    size_t sizeBytes,
    const std::wstring& activeManifestPath,
    CommandExecutionResult* outResult) {
    if (!outResult) {
        return;
    }

    uint32_t emitterId = 0u;
    std::string error;
    if (!TryResolveRemoveParticleEmitterCommand(raw, sizeBytes, &emitterId, &error)) {
        outResult->lastError = error.empty() ? "failed to resolve remove_particle_emitter command" : error;
        outResult->droppedCommands += 1;
        return;
    }

    if (!RemoveRetainedParticleEmitter(activeManifestPath, emitterId, &error)) {
        outResult->lastError = error.empty() ? "failed to remove retained particle emitter" : error;
        outResult->droppedCommands += 1;
        return;
    }

    outResult->executedParticleEmitterRemoveCommands += 1;
    outResult->renderedAny = true;
}

void ExecuteUpsertRibbonTrail(
    const uint8_t* raw,
    size_t sizeBytes,
    const EffectConfig& config,
    const std::wstring& activeManifestPath,
    CommandExecutionResult* outResult) {
    if (!outResult) {
        return;
    }

    ResolvedRibbonTrailCommand resolved{};
    std::string error;
    if (!TryResolveUpsertRibbonTrailCommand(raw, sizeBytes, config, &resolved, &error)) {
        outResult->lastError = error.empty() ? "failed to resolve upsert_ribbon_trail command" : error;
        outResult->droppedCommands += 1;
        return;
    }

    if (!UpsertRetainedRibbonTrail(activeManifestPath, resolved, &error)) {
        outResult->lastError = error.empty() ? "failed to upsert retained ribbon trail" : error;
        outResult->droppedCommands += 1;
        return;
    }

    outResult->executedRibbonTrailCommands += 1;
    outResult->renderedAny = true;
}

void ExecuteRemoveRibbonTrail(
    const uint8_t* raw,
    size_t sizeBytes,
    const std::wstring& activeManifestPath,
    CommandExecutionResult* outResult) {
    if (!outResult) {
        return;
    }

    uint32_t trailId = 0u;
    std::string error;
    if (!TryResolveRemoveRibbonTrailCommand(raw, sizeBytes, &trailId, &error)) {
        outResult->lastError = error.empty() ? "failed to resolve remove_ribbon_trail command" : error;
        outResult->droppedCommands += 1;
        return;
    }

    if (!RemoveRetainedRibbonTrail(activeManifestPath, trailId, &error)) {
        outResult->lastError = error.empty() ? "failed to remove retained ribbon trail" : error;
        outResult->droppedCommands += 1;
        return;
    }

    outResult->executedRibbonTrailRemoveCommands += 1;
    outResult->renderedAny = true;
}

void ExecuteUpsertQuadField(
    const uint8_t* raw,
    size_t sizeBytes,
    const EffectConfig& config,
    const std::wstring& activeManifestPath,
    CommandExecutionResult* outResult) {
    if (!outResult) {
        return;
    }

    ResolvedQuadFieldCommand resolved{};
    std::string error;
    if (!TryResolveUpsertQuadFieldCommand(raw, sizeBytes, config, activeManifestPath, false, &resolved, &error)) {
        outResult->lastError = error.empty() ? "failed to resolve upsert_quad_field command" : error;
        outResult->droppedCommands += 1;
        return;
    }

    if (!UpsertRetainedQuadField(activeManifestPath, resolved, &error)) {
        outResult->lastError = error.empty() ? "failed to upsert retained quad field" : error;
        outResult->droppedCommands += 1;
        return;
    }

    outResult->executedQuadFieldCommands += 1;
    outResult->renderedAny = true;
}

void ExecuteRemoveQuadField(
    const uint8_t* raw,
    size_t sizeBytes,
    const std::wstring& activeManifestPath,
    CommandExecutionResult* outResult) {
    if (!outResult) {
        return;
    }

    uint32_t fieldId = 0u;
    std::string error;
    if (!TryResolveRemoveQuadFieldCommand(raw, sizeBytes, &fieldId, &error)) {
        outResult->lastError = error.empty() ? "failed to resolve remove_quad_field command" : error;
        outResult->droppedCommands += 1;
        return;
    }

    if (!RemoveRetainedQuadField(activeManifestPath, fieldId, &error)) {
        outResult->lastError = error.empty() ? "failed to remove retained quad field" : error;
        outResult->droppedCommands += 1;
        return;
    }

    outResult->executedQuadFieldRemoveCommands += 1;
    outResult->renderedAny = true;
}

void ExecuteRemoveGroup(
    const uint8_t* raw,
    size_t sizeBytes,
    const std::wstring& activeManifestPath,
    CommandExecutionResult* outResult) {
    if (!outResult) {
        return;
    }

    uint32_t groupId = 0u;
    std::string error;
    if (!TryResolveRemoveGroupCommand(raw, sizeBytes, &groupId, &error)) {
        outResult->lastError = error.empty() ? "failed to resolve remove_group command" : error;
        outResult->droppedCommands += 1;
        return;
    }

    RemoveGroupPresentation(activeManifestPath, groupId);
    RemoveGroupClipRect(activeManifestPath, groupId);
    RemoveGroupLayer(activeManifestPath, groupId);
    RemoveGroupLocalOrigin(activeManifestPath, groupId);
    RemoveGroupTransform(activeManifestPath, groupId);
    RemoveGroupMaterial(activeManifestPath, groupId);
    RemoveGroupPass(activeManifestPath, groupId);
    RemoveRetainedGlowEmittersByGroup(activeManifestPath, groupId);
    RemoveRetainedSpriteEmittersByGroup(activeManifestPath, groupId);
    RemoveRetainedParticleEmittersByGroup(activeManifestPath, groupId);
    RemoveRetainedRibbonTrailsByGroup(activeManifestPath, groupId);
    RemoveRetainedQuadFieldsByGroup(activeManifestPath, groupId);

    outResult->executedGroupRemoveCommands += 1;
    outResult->renderedAny = true;
}

void ExecuteUpsertGroupPresentation(
    const uint8_t* raw,
    size_t sizeBytes,
    const std::wstring& activeManifestPath,
    CommandExecutionResult* outResult) {
    if (!outResult) {
        return;
    }

    ResolvedGroupPresentationCommand resolved{};
    std::string error;
    if (!TryResolveUpsertGroupPresentationCommand(raw, sizeBytes, &resolved, &error)) {
        outResult->lastError = error.empty() ? "failed to resolve upsert_group_presentation command" : error;
        outResult->droppedCommands += 1;
        return;
    }

    UpsertGroupPresentation(
        activeManifestPath,
        resolved.groupId,
        resolved.alphaMultiplier,
        resolved.visible);
    ApplyRetainedGlowEmitterGroupPresentation(
        activeManifestPath,
        resolved.groupId,
        resolved.alphaMultiplier,
        resolved.visible);
    ApplyRetainedSpriteEmitterGroupPresentation(
        activeManifestPath,
        resolved.groupId,
        resolved.alphaMultiplier,
        resolved.visible);
    ApplyRetainedParticleEmitterGroupPresentation(
        activeManifestPath,
        resolved.groupId,
        resolved.alphaMultiplier,
        resolved.visible);
    ApplyRetainedRibbonTrailGroupPresentation(
        activeManifestPath,
        resolved.groupId,
        resolved.alphaMultiplier,
        resolved.visible);
    ApplyRetainedQuadFieldGroupPresentation(
        activeManifestPath,
        resolved.groupId,
        resolved.alphaMultiplier,
        resolved.visible);

    outResult->executedGroupPresentationCommands += 1;
    outResult->renderedAny = true;
}

void ExecuteUpsertGroupClipRect(
    const uint8_t* raw,
    size_t sizeBytes,
    const std::wstring& activeManifestPath,
    CommandExecutionResult* outResult) {
    if (!outResult) {
        return;
    }

    ResolvedGroupClipRectCommand resolved{};
    std::string error;
    if (!TryResolveUpsertGroupClipRectCommand(raw, sizeBytes, &resolved, &error)) {
        outResult->lastError = error.empty() ? "failed to resolve upsert_group_clip_rect command" : error;
        outResult->droppedCommands += 1;
        return;
    }

    if (resolved.enabled) {
        UpsertGroupClipRect(
            activeManifestPath,
            resolved.groupId,
            resolved.clipRect,
            resolved.maskShapeKind,
            resolved.cornerRadiusPx);
    } else {
        RemoveGroupClipRect(activeManifestPath, resolved.groupId);
    }
    ApplyRetainedGlowEmitterGroupClipRect(activeManifestPath, resolved.groupId, resolved.clipRect);
    ApplyRetainedSpriteEmitterGroupClipRect(activeManifestPath, resolved.groupId, resolved.clipRect);
    ApplyRetainedParticleEmitterGroupClipRect(activeManifestPath, resolved.groupId, resolved.clipRect);
    ApplyRetainedRibbonTrailGroupClipRect(activeManifestPath, resolved.groupId, resolved.clipRect);
    ApplyRetainedQuadFieldGroupClipRect(activeManifestPath, resolved.groupId, resolved.clipRect);

    outResult->executedGroupClipRectCommands += 1;
    outResult->renderedAny = true;
}

void ExecuteUpsertGroupLayer(
    const uint8_t* raw,
    size_t sizeBytes,
    const std::wstring& activeManifestPath,
    CommandExecutionResult* outResult) {
    if (!outResult) {
        return;
    }

    ResolvedGroupLayerCommand resolved{};
    std::string error;
    if (!TryResolveUpsertGroupLayerCommand(raw, sizeBytes, &resolved, &error)) {
        outResult->lastError = error.empty() ? "failed to resolve upsert_group_layer command" : error;
        outResult->droppedCommands += 1;
        return;
    }

    UpsertGroupLayer(
        activeManifestPath,
        resolved.groupId,
        resolved.hasBlendOverride,
        resolved.blendMode,
        resolved.sortBias);
    ApplyRetainedGlowEmitterGroupLayer(activeManifestPath, resolved.groupId, resolved.hasBlendOverride, resolved.blendMode, resolved.sortBias);
    ApplyRetainedSpriteEmitterGroupLayer(activeManifestPath, resolved.groupId, resolved.hasBlendOverride, resolved.blendMode, resolved.sortBias);
    ApplyRetainedParticleEmitterGroupLayer(activeManifestPath, resolved.groupId, resolved.hasBlendOverride, resolved.blendMode, resolved.sortBias);
    ApplyRetainedRibbonTrailGroupLayer(activeManifestPath, resolved.groupId, resolved.hasBlendOverride, resolved.blendMode, resolved.sortBias);
    ApplyRetainedQuadFieldGroupLayer(activeManifestPath, resolved.groupId, resolved.hasBlendOverride, resolved.blendMode, resolved.sortBias);

    outResult->executedGroupLayerCommands += 1;
    outResult->renderedAny = true;
}

void ExecuteUpsertGroupTransform(
    const uint8_t* raw,
    size_t sizeBytes,
    const std::wstring& activeManifestPath,
    CommandExecutionResult* outResult) {
    if (!outResult) {
        return;
    }

    ResolvedGroupTransformCommand resolved{};
    std::string error;
    if (!TryResolveUpsertGroupTransformCommand(raw, sizeBytes, &resolved, &error)) {
        outResult->lastError = error.empty() ? "failed to resolve upsert_group_transform command" : error;
        outResult->droppedCommands += 1;
        return;
    }

    UpsertGroupTransform(
        activeManifestPath,
        resolved.groupId,
        resolved.offsetXPx,
        resolved.offsetYPx,
        resolved.rotationRad,
        resolved.uniformScale,
        resolved.pivotXPx,
        resolved.pivotYPx,
        resolved.scaleX,
        resolved.scaleY);
    ApplyRetainedGlowEmitterGroupTransform(
        activeManifestPath,
        resolved.groupId,
        resolved.offsetXPx,
        resolved.offsetYPx,
        resolved.rotationRad,
        resolved.uniformScale);
    ApplyRetainedSpriteEmitterGroupTransform(
        activeManifestPath,
        resolved.groupId,
        resolved.offsetXPx,
        resolved.offsetYPx,
        resolved.rotationRad,
        resolved.uniformScale);
    ApplyRetainedParticleEmitterGroupTransform(
        activeManifestPath,
        resolved.groupId,
        resolved.offsetXPx,
        resolved.offsetYPx,
        resolved.rotationRad,
        resolved.uniformScale);
    ApplyRetainedRibbonTrailGroupTransform(
        activeManifestPath,
        resolved.groupId,
        resolved.offsetXPx,
        resolved.offsetYPx,
        resolved.rotationRad,
        resolved.uniformScale);
    ApplyRetainedQuadFieldGroupTransform(
        activeManifestPath,
        resolved.groupId,
        resolved.offsetXPx,
        resolved.offsetYPx,
        resolved.rotationRad,
        resolved.uniformScale);

    outResult->executedGroupTransformCommands += 1;
    outResult->renderedAny = true;
}

void ExecuteUpsertGroupLocalOrigin(
    const uint8_t* raw,
    size_t sizeBytes,
    const std::wstring& activeManifestPath,
    CommandExecutionResult* outResult) {
    if (!outResult) {
        return;
    }

    ResolvedGroupLocalOriginCommand resolved{};
    std::string error;
    if (!TryResolveUpsertGroupLocalOriginCommand(raw, sizeBytes, &resolved, &error)) {
        outResult->lastError = error.empty() ? "failed to resolve upsert_group_local_origin command" : error;
        outResult->droppedCommands += 1;
        return;
    }

    UpsertGroupLocalOrigin(
        activeManifestPath,
        resolved.groupId,
        resolved.originXPx,
        resolved.originYPx);
    ApplyRetainedGlowEmitterGroupLocalOrigin(activeManifestPath, resolved.groupId, resolved.originXPx, resolved.originYPx);
    ApplyRetainedSpriteEmitterGroupLocalOrigin(activeManifestPath, resolved.groupId, resolved.originXPx, resolved.originYPx);
    ApplyRetainedParticleEmitterGroupLocalOrigin(activeManifestPath, resolved.groupId, resolved.originXPx, resolved.originYPx);
    ApplyRetainedRibbonTrailGroupLocalOrigin(activeManifestPath, resolved.groupId, resolved.originXPx, resolved.originYPx);
    ApplyRetainedQuadFieldGroupLocalOrigin(activeManifestPath, resolved.groupId, resolved.originXPx, resolved.originYPx);

    outResult->executedGroupLocalOriginCommands += 1;
    outResult->renderedAny = true;
}

void ExecuteUpsertGroupMaterial(
    const uint8_t* raw,
    size_t sizeBytes,
    const std::wstring& activeManifestPath,
    CommandExecutionResult* outResult) {
    if (!outResult) {
        return;
    }

    ResolvedGroupMaterialCommand resolved{};
    std::string error;
    if (!TryResolveUpsertGroupMaterialCommand(raw, sizeBytes, &resolved, &error)) {
        outResult->lastError = error.empty() ? "failed to resolve upsert_group_material command" : error;
        outResult->droppedCommands += 1;
        return;
    }

    UpsertGroupMaterial(
        activeManifestPath,
        resolved.groupId,
        resolved.hasTintOverride,
        resolved.tintArgb,
        resolved.intensityMultiplier,
        resolved.styleKind,
        resolved.styleAmount,
        resolved.diffusionAmount,
        resolved.persistenceAmount,
        resolved.echoAmount,
        resolved.echoDriftPx,
        resolved.feedbackMode,
        resolved.feedbackPhaseRad,
        resolved.feedbackLayerCount,
        resolved.feedbackLayerFalloff);
    ApplyRetainedGlowEmitterGroupMaterial(
        activeManifestPath,
        resolved.groupId,
        resolved.hasTintOverride,
        resolved.tintArgb,
        resolved.intensityMultiplier,
        resolved.styleKind,
        resolved.styleAmount,
        resolved.diffusionAmount,
        resolved.persistenceAmount,
        resolved.echoAmount,
        resolved.echoDriftPx,
        resolved.feedbackMode,
        resolved.feedbackPhaseRad,
        resolved.feedbackLayerCount,
        resolved.feedbackLayerFalloff);
    ApplyRetainedSpriteEmitterGroupMaterial(
        activeManifestPath,
        resolved.groupId,
        resolved.hasTintOverride,
        resolved.tintArgb,
        resolved.intensityMultiplier,
        resolved.styleKind,
        resolved.styleAmount,
        resolved.diffusionAmount,
        resolved.persistenceAmount,
        resolved.echoAmount,
        resolved.echoDriftPx,
        resolved.feedbackMode,
        resolved.feedbackPhaseRad,
        resolved.feedbackLayerCount,
        resolved.feedbackLayerFalloff);
    ApplyRetainedParticleEmitterGroupMaterial(
        activeManifestPath,
        resolved.groupId,
        resolved.hasTintOverride,
        resolved.tintArgb,
        resolved.intensityMultiplier,
        resolved.styleKind,
        resolved.styleAmount,
        resolved.diffusionAmount,
        resolved.persistenceAmount,
        resolved.echoAmount,
        resolved.echoDriftPx,
        resolved.feedbackMode,
        resolved.feedbackPhaseRad,
        resolved.feedbackLayerCount,
        resolved.feedbackLayerFalloff);
    ApplyRetainedRibbonTrailGroupMaterial(
        activeManifestPath,
        resolved.groupId,
        resolved.hasTintOverride,
        resolved.tintArgb,
        resolved.intensityMultiplier,
        resolved.styleKind,
        resolved.styleAmount,
        resolved.diffusionAmount,
        resolved.persistenceAmount,
        resolved.echoAmount,
        resolved.echoDriftPx,
        resolved.feedbackMode,
        resolved.feedbackPhaseRad,
        resolved.feedbackLayerCount,
        resolved.feedbackLayerFalloff);
    ApplyRetainedQuadFieldGroupMaterial(
        activeManifestPath,
        resolved.groupId,
        resolved.hasTintOverride,
        resolved.tintArgb,
        resolved.intensityMultiplier,
        resolved.styleKind,
        resolved.styleAmount,
        resolved.diffusionAmount,
        resolved.persistenceAmount,
        resolved.echoAmount,
        resolved.echoDriftPx,
        resolved.feedbackMode,
        resolved.feedbackPhaseRad,
        resolved.feedbackLayerCount,
        resolved.feedbackLayerFalloff);

    outResult->executedGroupMaterialCommands += 1;
    outResult->renderedAny = true;
}

void ExecuteUpsertGroupPass(
    const uint8_t* raw,
    size_t sizeBytes,
    const std::wstring& activeManifestPath,
    CommandExecutionResult* outResult) {
    if (!outResult) {
        return;
    }

    ResolvedGroupPassCommand resolved{};
    std::string error;
    if (!TryResolveUpsertGroupPassCommand(raw, sizeBytes, &resolved, &error)) {
        outResult->lastError = error.empty() ? "failed to resolve upsert_group_pass command" : error;
        outResult->droppedCommands += 1;
        return;
    }

    UpsertGroupPass(
        activeManifestPath,
        resolved.groupId,
        resolved.passKind,
        resolved.passAmount,
        resolved.responseAmount,
        resolved.secondaryStage,
        resolved.tertiaryStage,
        resolved.passMode,
        resolved.phaseRad,
        resolved.feedbackLayerCount,
        resolved.feedbackLayerFalloff);
    ApplyRetainedGlowEmitterGroupPass(
        activeManifestPath,
        resolved.groupId,
        resolved.passKind,
        resolved.passAmount,
        resolved.responseAmount,
        resolved.passMode,
        resolved.phaseRad,
        resolved.feedbackLayerCount,
        resolved.feedbackLayerFalloff);
    ApplyRetainedSpriteEmitterGroupPass(
        activeManifestPath,
        resolved.groupId,
        resolved.passKind,
        resolved.passAmount,
        resolved.responseAmount,
        resolved.passMode,
        resolved.phaseRad,
        resolved.feedbackLayerCount,
        resolved.feedbackLayerFalloff);
    ApplyRetainedParticleEmitterGroupPass(
        activeManifestPath,
        resolved.groupId,
        resolved.passKind,
        resolved.passAmount,
        resolved.responseAmount,
        resolved.passMode,
        resolved.phaseRad,
        resolved.feedbackLayerCount,
        resolved.feedbackLayerFalloff);
    ApplyRetainedRibbonTrailGroupPass(
        activeManifestPath,
        resolved.groupId,
        resolved.passKind,
        resolved.passAmount,
        resolved.responseAmount,
        resolved.passMode,
        resolved.phaseRad,
        resolved.feedbackLayerCount,
        resolved.feedbackLayerFalloff);
    ApplyRetainedQuadFieldGroupPass(
        activeManifestPath,
        resolved.groupId,
        resolved.passKind,
        resolved.passAmount,
        resolved.responseAmount,
        resolved.passMode,
        resolved.phaseRad,
        resolved.feedbackLayerCount,
        resolved.feedbackLayerFalloff);

    outResult->executedGroupPassCommands += 1;
    outResult->renderedAny = true;
}

} // namespace

CommandExecutionResult WasmClickCommandExecutor::Execute(
    const uint8_t* commandBuffer,
    size_t commandBytes,
    const EffectConfig& config,
    const std::wstring& activeManifestPath) {
    CommandExecutionResult result{};
    if (!commandBuffer || commandBytes == 0) {
        return result;
    }

    const CommandParseResult parsed = WasmCommandBufferParser::Parse(
        commandBuffer, commandBytes, 4096u);
    result.parsedCommands = static_cast<uint32_t>(parsed.commands.size());
    if (parsed.error != CommandParseError::None) {
        result.lastError = std::string("command parse failed: ") + CommandParseErrorToString(parsed.error);
        result.droppedCommands = result.parsedCommands;
        return result;
    }

    for (const auto& record : parsed.commands) {
        if (record.offsetBytes + record.sizeBytes > commandBytes) {
            result.droppedCommands += 1;
            continue;
        }

        const uint8_t* raw = commandBuffer + record.offsetBytes;
        switch (record.kind) {
        case CommandKind::SpawnText: {
            SpawnTextCommandV1 cmd{};
            std::memcpy(&cmd, raw, sizeof(cmd));
            ExecuteSpawnText(cmd, config, &result);
            break;
        }
        case CommandKind::SpawnImage: {
            SpawnImageCommandV1 cmd{};
            std::memcpy(&cmd, raw, sizeof(cmd));
            ExecuteSpawnImage(ResolveSpawnImageCommand(cmd), config, activeManifestPath, &result);
            break;
        }
        case CommandKind::SpawnImageAffine: {
            SpawnImageAffineCommandV1 cmd{};
            std::memcpy(&cmd, raw, sizeof(cmd));
            ExecuteSpawnImage(ResolveSpawnImageCommand(cmd), config, activeManifestPath, &result);
            break;
        }
        case CommandKind::SpawnPulse: {
            SpawnPulseCommandV1 cmd{};
            std::memcpy(&cmd, raw, sizeof(cmd));
            ExecuteSpawnPulse(cmd, config, &result);
            break;
        }
        case CommandKind::SpawnPolyline:
            ExecuteSpawnPolyline(raw, record.sizeBytes, config, &result);
            break;
        case CommandKind::SpawnPathStroke:
            ExecuteSpawnPathStroke(raw, record.sizeBytes, config, &result);
            break;
        case CommandKind::SpawnPathFill:
            ExecuteSpawnPathFill(raw, record.sizeBytes, config, &result);
            break;
        case CommandKind::SpawnGlowBatch:
            ExecuteSpawnGlowBatch(raw, record.sizeBytes, config, &result);
            break;
        case CommandKind::SpawnSpriteBatch:
            ExecuteSpawnSpriteBatch(raw, record.sizeBytes, config, activeManifestPath, &result);
            break;
        case CommandKind::SpawnQuadBatch:
            ExecuteSpawnQuadBatch(raw, record.sizeBytes, config, activeManifestPath, &result);
            break;
        case CommandKind::SpawnRibbonStrip:
            ExecuteSpawnRibbonStrip(raw, record.sizeBytes, config, &result);
            break;
        case CommandKind::UpsertGlowEmitter:
            ExecuteUpsertGlowEmitter(raw, record.sizeBytes, config, activeManifestPath, &result);
            break;
        case CommandKind::RemoveGlowEmitter:
            ExecuteRemoveGlowEmitter(raw, record.sizeBytes, activeManifestPath, &result);
            break;
        case CommandKind::UpsertSpriteEmitter:
            ExecuteUpsertSpriteEmitter(raw, record.sizeBytes, config, activeManifestPath, &result);
            break;
        case CommandKind::RemoveSpriteEmitter:
            ExecuteRemoveSpriteEmitter(raw, record.sizeBytes, activeManifestPath, &result);
            break;
        case CommandKind::UpsertParticleEmitter:
            ExecuteUpsertParticleEmitter(raw, record.sizeBytes, config, activeManifestPath, &result);
            break;
        case CommandKind::RemoveParticleEmitter:
            ExecuteRemoveParticleEmitter(raw, record.sizeBytes, activeManifestPath, &result);
            break;
        case CommandKind::UpsertRibbonTrail:
            ExecuteUpsertRibbonTrail(raw, record.sizeBytes, config, activeManifestPath, &result);
            break;
        case CommandKind::RemoveRibbonTrail:
            ExecuteRemoveRibbonTrail(raw, record.sizeBytes, activeManifestPath, &result);
            break;
        case CommandKind::UpsertQuadField:
            ExecuteUpsertQuadField(raw, record.sizeBytes, config, activeManifestPath, &result);
            break;
        case CommandKind::RemoveQuadField:
            ExecuteRemoveQuadField(raw, record.sizeBytes, activeManifestPath, &result);
            break;
        case CommandKind::RemoveGroup:
            ExecuteRemoveGroup(raw, record.sizeBytes, activeManifestPath, &result);
            break;
        case CommandKind::UpsertGroupPresentation:
            ExecuteUpsertGroupPresentation(raw, record.sizeBytes, activeManifestPath, &result);
            break;
        case CommandKind::UpsertGroupClipRect:
            ExecuteUpsertGroupClipRect(raw, record.sizeBytes, activeManifestPath, &result);
            break;
        case CommandKind::UpsertGroupLayer:
            ExecuteUpsertGroupLayer(raw, record.sizeBytes, activeManifestPath, &result);
            break;
        case CommandKind::UpsertGroupTransform:
            ExecuteUpsertGroupTransform(raw, record.sizeBytes, activeManifestPath, &result);
            break;
        case CommandKind::UpsertGroupLocalOrigin:
            ExecuteUpsertGroupLocalOrigin(raw, record.sizeBytes, activeManifestPath, &result);
            break;
        case CommandKind::UpsertGroupMaterial:
            ExecuteUpsertGroupMaterial(raw, record.sizeBytes, activeManifestPath, &result);
            break;
        case CommandKind::UpsertGroupPass:
            ExecuteUpsertGroupPass(raw, record.sizeBytes, activeManifestPath, &result);
            break;
        default:
            result.droppedCommands += 1;
            break;
        }
    }

    return result;
}

} // namespace mousefx::wasm
