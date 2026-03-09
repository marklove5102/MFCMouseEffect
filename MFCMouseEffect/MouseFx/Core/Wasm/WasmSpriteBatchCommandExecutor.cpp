#include "pch.h"

#include "MouseFx/Core/Wasm/WasmSpriteBatchCommandExecutor.h"

#include "MouseFx/Core/Overlay/OverlayHostService.h"
#include "MouseFx/Core/Wasm/WasmQuadBatchCommandConfig.h"
#include "MouseFx/Core/Wasm/WasmSpriteBatchCommandConfig.h"
#include "MouseFx/Core/Wasm/WasmSpriteBatchRenderShared.h"
#include "MouseFx/Interfaces/IRippleRenderer.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>
#include <vector>

namespace mousefx::wasm {

namespace {

class SpawnSpriteBatchRenderer final : public IRippleRenderer {
public:
    explicit SpawnSpriteBatchRenderer(
        std::vector<ResolvedSpriteBatchItem> items,
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

        const float clampedT = sprite_batch_render_shared::Clamp01(t);
        const float eased = 1.0f - (1.0f - clampedT) * (1.0f - clampedT) * (1.0f - clampedT);
        const float fade = std::max(0.0f, 1.0f - eased);
        const float elapsedSec = static_cast<float>(elapsedMs) / 1000.0f;
        g.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
        g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);

        for (const ResolvedSpriteBatchItem& item : items_) {
            const float baseAlpha = item.alpha * fade;
            if (baseAlpha <= 0.001f || item.widthPx <= 0.5f || item.heightPx <= 0.5f) {
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
                baseAlpha,
                screenBlend_,
                &cache_);
        }
    }

private:
    std::vector<ResolvedSpriteBatchItem> items_{};
    bool screenBlend_ = false;
    sprite_batch_render_shared::BitmapCache cache_{};
};

} // namespace

void ExecuteResolvedSpawnSpriteBatch(
    ResolvedSpawnSpriteBatchCommand resolved,
    CommandExecutionResult* outResult,
    const char* failureMessage) {
    if (!outResult) {
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

    auto renderer = std::make_unique<SpawnSpriteBatchRenderer>(
        std::move(resolved.items),
        UsesScreenLikeBlend(resolved.semantics.blendMode));
    const uint64_t id = OverlayHostService::Instance().ShowRipple(
        ev,
        style,
        std::move(renderer),
        renderParams);
    if (id == 0) {
        outResult->lastError = failureMessage ? failureMessage : "failed to render sprite batch command";
        outResult->droppedCommands += 1;
        return;
    }

    outResult->executedSpriteBatchCommands += 1;
    outResult->renderedAny = true;
}

void ExecuteSpawnSpriteBatch(
    const uint8_t* raw,
    size_t sizeBytes,
    const EffectConfig& config,
    const std::wstring& activeManifestPath,
    CommandExecutionResult* outResult) {
    if (!outResult) {
        return;
    }

    ResolvedSpawnSpriteBatchCommand resolved{};
    std::string error;
    if (!TryResolveSpawnSpriteBatchCommand(raw, sizeBytes, config, activeManifestPath, false, &resolved, &error)) {
        outResult->lastError = error.empty() ? "failed to resolve spawn_sprite_batch command" : error;
        outResult->droppedCommands += 1;
        return;
    }

    ExecuteResolvedSpawnSpriteBatch(std::move(resolved), outResult, "failed to render spawn_sprite_batch command");
}

void ExecuteSpawnQuadBatch(
    const uint8_t* raw,
    size_t sizeBytes,
    const EffectConfig& config,
    const std::wstring& activeManifestPath,
    CommandExecutionResult* outResult) {
    if (!outResult) {
        return;
    }

    ResolvedSpawnSpriteBatchCommand resolved{};
    std::string error;
    if (!TryResolveSpawnQuadBatchCommand(raw, sizeBytes, config, activeManifestPath, false, &resolved, &error)) {
        outResult->lastError = error.empty() ? "failed to resolve spawn_quad_batch command" : error;
        outResult->droppedCommands += 1;
        return;
    }

    ExecuteResolvedSpawnSpriteBatch(std::move(resolved), outResult, "failed to render spawn_quad_batch command");
}

} // namespace mousefx::wasm
