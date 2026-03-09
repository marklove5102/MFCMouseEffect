#include "pch.h"

#include "WasmPathFillCommandExecutor.h"

#include "MouseFx/Core/Overlay/OverlayHostService.h"
#include "MouseFx/Core/Wasm/WasmPathFillCommandConfig.h"
#include "MouseFx/Core/Wasm/WasmPathGraphicsPath.h"
#include "MouseFx/Interfaces/IRippleRenderer.h"
#include "MouseFx/Renderers/RenderUtils.h"

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

namespace mousefx::wasm {

namespace {

class SpawnPathFillRenderer final : public IRippleRenderer {
public:
    SpawnPathFillRenderer(
        std::vector<ResolvedPathFillNode> localNodes,
        float glowWidthPx,
        uint32_t fillArgb,
        uint32_t glowArgb,
        uint8_t fillRule)
        : localNodes_(std::move(localNodes)),
          glowWidthPx_(glowWidthPx),
          fillArgb_(fillArgb),
          glowArgb_(glowArgb),
          fillRule_(fillRule) {}

    void Render(
        Gdiplus::Graphics& g,
        float t,
        uint64_t,
        int,
        const RippleStyle&) override {
        Gdiplus::GraphicsPath path;
        if (!BuildPathGraphicsPath(localNodes_, ResolvePathFillMode(fillRule_), &path)) {
            return;
        }

        const float clampedT = render_utils::Clamp01(t);
        const float eased = 1.0f - (1.0f - clampedT) * (1.0f - clampedT) * (1.0f - clampedT);
        const float alpha = 1.0f - eased;
        const Gdiplus::Color fillBase = render_utils::ToGdiPlus({fillArgb_});
        const Gdiplus::Color glowBase = render_utils::ToGdiPlus({glowArgb_});

        g.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
        if (glowWidthPx_ > 0.0f) {
            for (int glowPass = 0; glowPass < 3; ++glowPass) {
                const float width = glowWidthPx_ + static_cast<float>(glowPass) * 5.0f;
                const BYTE glowAlpha = render_utils::ClampByte(static_cast<int>(
                    static_cast<float>(glowBase.GetA()) * alpha * (0.30f - static_cast<float>(glowPass) * 0.07f)));
                Gdiplus::Pen glowPen(
                    Gdiplus::Color(glowAlpha, glowBase.GetR(), glowBase.GetG(), glowBase.GetB()),
                    std::max(1.0f, width));
                glowPen.SetStartCap(Gdiplus::LineCapRound);
                glowPen.SetEndCap(Gdiplus::LineCapRound);
                glowPen.SetLineJoin(Gdiplus::LineJoinRound);
                g.DrawPath(&glowPen, &path);
            }
        }

        const BYTE fillAlpha = render_utils::ClampByte(
            static_cast<int>(static_cast<float>(fillBase.GetA()) * alpha));
        Gdiplus::SolidBrush fillBrush(
            Gdiplus::Color(fillAlpha, fillBase.GetR(), fillBase.GetG(), fillBase.GetB()));
        g.FillPath(&fillBrush, &path);
    }

private:
    std::vector<ResolvedPathFillNode> localNodes_{};
    float glowWidthPx_ = 10.0f;
    uint32_t fillArgb_ = 0xFFFFFFFFu;
    uint32_t glowArgb_ = 0x66FFFFFFu;
    uint8_t fillRule_ = kPathFillRuleNonZero;
};

} // namespace

void ExecuteResolvedPathFill(
    ResolvedSpawnPathFillCommand resolved,
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
    style.strokeWidth = std::max(1.0f, resolved.glowWidthPx);
    style.fill = {resolved.fillArgb};
    style.stroke = {resolved.fillArgb};
    style.glow = {resolved.glowArgb};

    RenderParams renderParams{};
    renderParams.loop = false;
    renderParams.intensity = resolved.alpha;
    renderParams.startDelayMs = resolved.delayMs;
    renderParams.semantics = resolved.semantics;

    auto renderer = std::make_unique<SpawnPathFillRenderer>(
        std::move(resolved.localNodes),
        resolved.glowWidthPx,
        resolved.fillArgb,
        resolved.glowArgb,
        resolved.fillRule);
    const uint64_t id = OverlayHostService::Instance().ShowRipple(
        ev,
        style,
        std::move(renderer),
        renderParams);
    if (id == 0) {
        outResult->lastError = failureMessage ? failureMessage : "failed to render spawn_path_fill command";
        outResult->droppedCommands += 1;
        return;
    }

    outResult->executedPathFillCommands += 1;
    outResult->renderedAny = true;
}

void ExecuteSpawnPathFill(
    const uint8_t* raw,
    size_t sizeBytes,
    const EffectConfig& config,
    CommandExecutionResult* outResult) {
    if (!outResult) {
        return;
    }

    ResolvedSpawnPathFillCommand resolved{};
    std::string error;
    if (!TryResolveSpawnPathFillCommand(raw, sizeBytes, config, &resolved, &error)) {
        outResult->lastError = error.empty() ? "failed to resolve spawn_path_fill command" : error;
        outResult->droppedCommands += 1;
        return;
    }

    ExecuteResolvedPathFill(std::move(resolved), outResult, "failed to render spawn_path_fill command");
}

} // namespace mousefx::wasm
