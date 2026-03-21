#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionPresenter.h"

#include "MouseFx/Utils/StringUtils.h"
#include "Platform/PlatformDisplayTopology.h"

#include <algorithm>

namespace mousefx::windows {
namespace {

int WidthOf(const platform::DisplayRect& rect) {
    return rect.right - rect.left;
}

int HeightOf(const platform::DisplayRect& rect) {
    return rect.bottom - rect.top;
}

platform::DisplayRect ResolveTargetBounds(const Win32MouseCompanionVisualState& state) {
    return platform::ResolveTargetDisplayMonitorBounds(
        state.config.targetMonitor,
        platform::DisplayPoint{state.lastPoint.x, state.lastPoint.y});
}

ScreenPoint ResolveAnchorPoint(const Win32MouseCompanionVisualState& state) {
    if (state.hasLastPoint) {
        return state.lastPoint;
    }
    const platform::DisplayRect bounds = ResolveTargetBounds(state);
    ScreenPoint fallback{};
    fallback.x = bounds.left + WidthOf(bounds) / 2;
    fallback.y = bounds.top + HeightOf(bounds) / 2;
    return fallback;
}

} // namespace

RECT Win32MouseCompanionPresenter::ResolveWindowBounds(const Win32MouseCompanionVisualState& state) const {
    const int sizePx = (std::max)(48, state.config.sizePx);
    const platform::DisplayRect targetBounds = ResolveTargetBounds(state);
    const ScreenPoint anchorPoint = ResolveAnchorPoint(state);
    RECT rect{};

    const std::string positionMode = ToLowerAscii(TrimAscii(state.config.positionMode));
    if (positionMode == "absolute") {
        rect.left = targetBounds.left + state.config.absoluteX;
        rect.top = targetBounds.top + state.config.absoluteY;
    } else if (positionMode == "fixed_bottom_left") {
        rect.left = targetBounds.left + state.config.offsetX;
        rect.top = targetBounds.bottom - sizePx - state.config.offsetY;
    } else {
        rect.left = anchorPoint.x + state.config.offsetX;
        rect.top = anchorPoint.y + state.config.offsetY;
    }
    rect.right = rect.left + sizePx;
    rect.bottom = rect.top + sizePx;
    ApplyClampMode(&rect, state);
    return rect;
}

void Win32MouseCompanionPresenter::ApplyClampMode(RECT* rect, const Win32MouseCompanionVisualState& state) const {
    if (!rect) {
        return;
    }
    const platform::DisplayRect bounds = ResolveTargetBounds(state);
    const std::string clampMode = ToLowerAscii(TrimAscii(state.config.edgeClampMode));
    if (clampMode == "free" || WidthOf(bounds) <= 0 || HeightOf(bounds) <= 0) {
        return;
    }

    const int width = rect->right - rect->left;
    const int height = rect->bottom - rect->top;
    const int minVisibleW = (clampMode == "soft") ? width / 2 : width;
    const int minVisibleH = (clampMode == "soft") ? height / 2 : height;

    const int minLeft = bounds.left - (width - minVisibleW);
    const int maxLeft = bounds.right - minVisibleW;
    const int minTop = bounds.top - (height - minVisibleH);
    const int maxTop = bounds.bottom - minVisibleH;

    rect->left = std::clamp<LONG>(rect->left, static_cast<LONG>(minLeft), static_cast<LONG>(maxLeft));
    rect->top = std::clamp<LONG>(rect->top, static_cast<LONG>(minTop), static_cast<LONG>(maxTop));
    rect->right = rect->left + width;
    rect->bottom = rect->top + height;
}

} // namespace mousefx::windows
