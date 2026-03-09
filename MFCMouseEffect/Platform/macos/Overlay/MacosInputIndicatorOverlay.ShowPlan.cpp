#include "pch.h"

#include "Platform/macos/Overlay/MacosInputIndicatorOverlay.ShowPlan.h"

#include "MouseFx/Core/Overlay/OverlayCoordSpace.h"
#include "MouseFx/Utils/StringUtils.h"
#include "Platform/macos/Overlay/MacosInputIndicatorOverlayInternals.h"

namespace mousefx::macos_input_indicator_detail {

namespace {

ScreenPoint ResolveTopLeftWindowOrigin(const ScreenPoint& topLeftPt, int sizePx) {
    const ScreenPoint cocoaTopLeft = ScreenToOverlayPoint(topLeftPt);
    ScreenPoint frameOrigin = cocoaTopLeft;
    frameOrigin.y -= sizePx;
    return frameOrigin;
}

} // namespace

IndicatorShowPlan BuildIndicatorShowPlan(const InputIndicatorConfig& cfg, ScreenPoint point) {
    IndicatorShowPlan plan{};
    plan.sizePx = macos_input_indicator::ClampInt(cfg.sizePx, 28, 220);
    plan.durationMs = macos_input_indicator::ClampInt(cfg.durationMs, 80, 5000);

    const bool absolute = (ToLowerAscii(TrimAscii(cfg.positionMode)) == "absolute");
    ScreenPoint targetTopLeft{};
    if (absolute) {
        targetTopLeft.x = cfg.absoluteX;
        targetTopLeft.y = cfg.absoluteY;
    } else {
        targetTopLeft.x = point.x + cfg.offsetX;
        targetTopLeft.y = point.y + cfg.offsetY;
    }

    const ScreenPoint frameOrigin = ResolveTopLeftWindowOrigin(targetTopLeft, plan.sizePx);
    plan.x = frameOrigin.x;
    plan.y = frameOrigin.y;
    return plan;
}

} // namespace mousefx::macos_input_indicator_detail
