#include "pch.h"

#include "WasmPathStrokeCommandExecutor.h"

#include "MouseFx/Core/Overlay/OverlayHostService.h"
#include "MouseFx/Core/Wasm/WasmPathGraphicsPath.h"
#include "MouseFx/Core/Wasm/WasmPathStrokeCommandConfig.h"
#include "MouseFx/Interfaces/IRippleRenderer.h"
#include "MouseFx/Renderers/RenderUtils.h"

#include <cmath>
#include <memory>
#include <utility>
#include <vector>

namespace mousefx::wasm {

namespace {

class SpawnPathStrokeRenderer final : public IRippleRenderer {
public:
    SpawnPathStrokeRenderer(
        std::vector<ResolvedPathStrokeNode> localNodes,
        float lineWidthPx,
        uint32_t strokeArgb,
        uint32_t glowArgb,
        uint8_t lineJoin,
        uint8_t lineCap)
        : localNodes_(std::move(localNodes)),
          lineWidthPx_(lineWidthPx),
          strokeArgb_(strokeArgb),
          glowArgb_(glowArgb),
          lineJoin_(lineJoin),
          lineCap_(lineCap) {}

    void Render(
        Gdiplus::Graphics& g,
        float t,
        uint64_t,
        int,
        const RippleStyle&) override {
        Gdiplus::GraphicsPath path;
        if (!BuildPathGraphicsPath(localNodes_, Gdiplus::FillModeWinding, &path)) {
            return;
        }

        const float clampedT = render_utils::Clamp01(t);
        const float eased = 1.0f - (1.0f - clampedT) * (1.0f - clampedT) * (1.0f - clampedT);
        const float alpha = 1.0f - eased;
        const Gdiplus::Color strokeBase = render_utils::ToGdiPlus({strokeArgb_});
        const Gdiplus::Color glowBase = render_utils::ToGdiPlus({glowArgb_});
        const Gdiplus::LineJoin lineJoin = ResolvePathStrokeLineJoin(lineJoin_);
        const Gdiplus::LineCap lineCap = ResolvePathStrokeLineCap(lineCap_);

        g.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
        for (int glowPass = 0; glowPass < 3; ++glowPass) {
            const float width = lineWidthPx_ + 10.0f + static_cast<float>(glowPass) * 4.0f;
            const BYTE glowAlpha = render_utils::ClampByte(static_cast<int>(
                static_cast<float>(glowBase.GetA()) * alpha * (0.34f - static_cast<float>(glowPass) * 0.08f)));
            Gdiplus::Pen glowPen(
                Gdiplus::Color(glowAlpha, glowBase.GetR(), glowBase.GetG(), glowBase.GetB()),
                width);
            glowPen.SetStartCap(lineCap);
            glowPen.SetEndCap(lineCap);
            glowPen.SetLineJoin(lineJoin);
            g.DrawPath(&glowPen, &path);
        }

        const BYTE strokeAlpha = render_utils::ClampByte(
            static_cast<int>(static_cast<float>(strokeBase.GetA()) * alpha));
        Gdiplus::Pen strokePen(
            Gdiplus::Color(strokeAlpha, strokeBase.GetR(), strokeBase.GetG(), strokeBase.GetB()),
            lineWidthPx_);
        strokePen.SetStartCap(lineCap);
        strokePen.SetEndCap(lineCap);
        strokePen.SetLineJoin(lineJoin);
        g.DrawPath(&strokePen, &path);
    }

private:
    std::vector<ResolvedPathStrokeNode> localNodes_{};
    float lineWidthPx_ = 4.0f;
    uint32_t strokeArgb_ = 0xFFFFFFFFu;
    uint32_t glowArgb_ = 0x66FFFFFFu;
    uint8_t lineJoin_ = kPathStrokeLineJoinRound;
    uint8_t lineCap_ = kPathStrokeLineCapRound;
};

} // namespace

void ExecuteSpawnPathStroke(
    const uint8_t* raw,
    size_t sizeBytes,
    const EffectConfig& config,
    CommandExecutionResult* outResult) {
    if (!outResult) {
        return;
    }

    ResolvedSpawnPathStrokeCommand resolved{};
    std::string error;
    if (!TryResolveSpawnPathStrokeCommand(raw, sizeBytes, config, &resolved, &error)) {
        outResult->lastError = error.empty() ? "failed to resolve spawn_path_stroke command" : error;
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
    renderParams.semantics = resolved.semantics;

    auto renderer = std::make_unique<SpawnPathStrokeRenderer>(
        std::move(resolved.localNodes),
        resolved.lineWidthPx,
        resolved.strokeArgb,
        resolved.glowArgb,
        resolved.lineJoin,
        resolved.lineCap);
    const uint64_t id = OverlayHostService::Instance().ShowRipple(
        ev,
        style,
        std::move(renderer),
        renderParams);
    if (id == 0) {
        outResult->lastError = "failed to render spawn_path_stroke command";
        outResult->droppedCommands += 1;
        return;
    }

    outResult->executedPathStrokeCommands += 1;
    outResult->renderedAny = true;
}

} // namespace mousefx::wasm
