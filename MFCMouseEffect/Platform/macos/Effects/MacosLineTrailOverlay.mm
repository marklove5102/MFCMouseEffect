#include "pch.h"

#include "Platform/macos/Effects/MacosLineTrailOverlay.h"

#include "Platform/macos/Effects/MacosOverlayRenderSupport.h"
#include "Platform/macos/Overlay/MacosOverlayCoordSpaceConversion.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <deque>
#include <mutex>

#if defined(__APPLE__)
#import <AppKit/AppKit.h>
#import <QuartzCore/QuartzCore.h>
#import <dispatch/dispatch.h>
#endif

namespace mousefx::macos_line_trail {

#if defined(__APPLE__)
namespace {

struct TrailPoint final {
    ScreenPoint pt{};
    uint64_t timeMs = 0;
};

struct LineTrailState final {
    NSWindow* window = nil;
    CAShapeLayer* pathLayer = nil;
    ScreenPoint windowOrigin{};
    std::deque<TrailPoint> points;
    LineTrailConfig config{};
    uint64_t lastTickMs = 0;
    uint64_t lastInputMs = 0;
    dispatch_source_t timer = nullptr;
    bool running = false;
};

LineTrailState& State() {
    static LineTrailState state{};
    return state;
}

uint64_t NowMs() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());
}

NSColor* ArgbToNsColor(uint32_t argb, double alphaScale) {
    const CGFloat baseAlpha = static_cast<CGFloat>((argb >> 24) & 0xFFu) / 255.0;
    const CGFloat alpha = static_cast<CGFloat>(std::clamp(baseAlpha * alphaScale, 0.0, 1.0));
    const CGFloat red = static_cast<CGFloat>((argb >> 16) & 0xFFu) / 255.0;
    const CGFloat green = static_cast<CGFloat>((argb >> 8) & 0xFFu) / 255.0;
    const CGFloat blue = static_cast<CGFloat>(argb & 0xFFu) / 255.0;
    return [NSColor colorWithCalibratedRed:red green:green blue:blue alpha:alpha];
}

void StopTimer(LineTrailState& state) {
    if (!state.timer) {
        return;
    }
    dispatch_source_cancel(state.timer);
    state.timer = nullptr;
}

void CloseWindow(LineTrailState& state) {
    if (state.window == nil) {
        return;
    }
    [state.window orderOut:nil];
    [state.window release];
    state.window = nil;
    state.pathLayer = nil;
}

void ResetState(LineTrailState& state) {
    StopTimer(state);
    CloseWindow(state);
    state.points.clear();
    state.lastInputMs = 0;
    state.lastTickMs = 0;
    state.points.clear();
    state.running = false;
    state.lastTickMs = 0;
    state.lastInputMs = 0;
}

NSScreen* ResolveScreenForPoint(const ScreenPoint& overlayPt) {
    NSArray<NSScreen*>* screens = [NSScreen screens];
    if (screens == nil || [screens count] == 0) {
        return [NSScreen mainScreen];
    }
    const NSPoint point = NSMakePoint(static_cast<CGFloat>(overlayPt.x), static_cast<CGFloat>(overlayPt.y));
    for (NSScreen* screen in screens) {
        if (NSPointInRect(point, [screen frame])) {
            return screen;
        }
    }
    NSScreen* main = [NSScreen mainScreen];
    return main != nil ? main : [screens objectAtIndex:0];
}

bool EnsureWindowForPoint(LineTrailState& state, const ScreenPoint& overlayPt) {
    NSScreen* screen = ResolveScreenForPoint(overlayPt);
    if (screen == nil) {
        return false;
    }

    const NSRect frame = [screen frame];
    if (frame.size.width <= 0.0 || frame.size.height <= 0.0) {
        return false;
    }

    const bool needsNewWindow =
        (state.window == nil) ||
        std::fabs(state.windowOrigin.x - frame.origin.x) > 0.5 ||
        std::fabs(state.windowOrigin.y - frame.origin.y) > 0.5 ||
        std::fabs([state.window frame].size.width - frame.size.width) > 0.5 ||
        std::fabs([state.window frame].size.height - frame.size.height) > 0.5;

    if (!needsNewWindow) {
        return true;
    }

    CloseWindow(state);

    NSWindow* window = macos_overlay_support::CreateOverlayWindow(frame);
    if (window == nil) {
        return false;
    }

    NSView* content = [window contentView];
    [content setWantsLayer:YES];
    macos_overlay_support::ApplyOverlayContentScale(content, overlayPt);

    CAShapeLayer* pathLayer = [CAShapeLayer layer];
    pathLayer.frame = content.bounds;
    pathLayer.fillColor = [[NSColor clearColor] CGColor];
    pathLayer.lineCap = kCALineCapRound;
    pathLayer.lineJoin = kCALineJoinRound;
    [content.layer addSublayer:pathLayer];

    state.window = window;
    state.pathLayer = pathLayer;
    state.windowOrigin.x = static_cast<int32_t>(std::lround(frame.origin.x));
    state.windowOrigin.y = static_cast<int32_t>(std::lround(frame.origin.y));

    [window orderFrontRegardless];
    return true;
}

float IdleFadeFactor(uint64_t now, uint64_t lastPointMs, const IdleFadeParams& idle) {
    const int startMs = idle.startMs > 0 ? idle.startMs : 60;
    int endMs = idle.endMs > 0 ? idle.endMs : 220;
    if (endMs <= startMs) {
        endMs = startMs + 1;
    }
    const uint64_t elapsed = (now > lastPointMs) ? (now - lastPointMs) : 0;
    if (elapsed <= static_cast<uint64_t>(startMs)) {
        return 1.0f;
    }
    if (elapsed >= static_cast<uint64_t>(endMs)) {
        return 0.0f;
    }
    const float t = static_cast<float>(elapsed - startMs) / static_cast<float>(endMs - startMs);
    return std::max(0.0f, 1.0f - t);
}

ScreenPoint ResolveOverlayPoint(const ScreenPoint& screenPt) {
    ScreenPoint overlayPt = screenPt;
    ScreenPoint cocoaPt{};
    if (macos_overlay_coord_conversion::TryConvertQuartzToCocoa(screenPt, &cocoaPt)) {
        overlayPt = cocoaPt;
    }
    return overlayPt;
}

void RebuildPath(LineTrailState& state, uint64_t nowMs) {
    if (state.pathLayer == nil) {
        return;
    }
    if (state.points.size() < 2) {
        state.pathLayer.path = nullptr;
        return;
    }

    CGMutablePathRef path = CGPathCreateMutable();
    for (size_t i = 0; i + 1 < state.points.size(); ++i) {
        const auto& p1 = state.points[i];
        const auto& p2 = state.points[i + 1];
        const CGFloat x1 = static_cast<CGFloat>(p1.pt.x - state.windowOrigin.x);
        const CGFloat y1 = static_cast<CGFloat>(p1.pt.y - state.windowOrigin.y);
        const CGFloat x2 = static_cast<CGFloat>(p2.pt.x - state.windowOrigin.x);
        const CGFloat y2 = static_cast<CGFloat>(p2.pt.y - state.windowOrigin.y);
        CGPathMoveToPoint(path, nullptr, x1, y1);
        CGPathAddLineToPoint(path, nullptr, x2, y2);
    }

    const uint64_t oldest = state.points.front().timeMs;
    const float life = std::max(0.0f, 1.0f - static_cast<float>((nowMs - oldest)) / std::max(1, state.config.durationMs));
    const float idleFactor = IdleFadeFactor(nowMs, state.points.back().timeMs, state.config.idleFade);
    const double alphaScale = std::clamp(static_cast<double>(life * idleFactor), 0.0, 1.0);

    state.pathLayer.path = path;
    CGPathRelease(path);
    state.pathLayer.strokeColor = [ArgbToNsColor(state.config.strokeArgb, alphaScale) CGColor];
    state.pathLayer.lineWidth = state.config.lineWidth;
}

void Tick(LineTrailState& state) {
    const uint64_t nowMs = NowMs();
    const uint64_t duration = static_cast<uint64_t>(std::max(1, state.config.durationMs));
    while (!state.points.empty()) {
        const uint64_t age = nowMs - state.points.front().timeMs;
        if (age <= duration) {
            break;
        }
        state.points.pop_front();
    }
    if (state.points.size() < 2) {
        state.pathLayer.path = nullptr;
    } else {
        RebuildPath(state, nowMs);
    }
    if (state.points.size() < 2) {
        if (state.lastInputMs == 0 || (nowMs - state.lastInputMs) > duration) {
            ResetState(state);
        }
    }
}

void EnsureTimer(LineTrailState& state) {
    if (state.timer) {
        return;
    }
    dispatch_source_t timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, dispatch_get_main_queue());
    if (!timer) {
        return;
    }
    dispatch_source_set_timer(timer, dispatch_time(DISPATCH_TIME_NOW, 0), static_cast<uint64_t>(33) * NSEC_PER_MSEC, 2 * NSEC_PER_MSEC);
    dispatch_source_set_event_handler(timer, ^{
      Tick(State());
    });
    dispatch_resume(timer);
    state.timer = timer;
}

void UpdateLineTrailOnMain(const ScreenPoint& screenPt, const LineTrailConfig& config) {
    LineTrailState& state = State();
    state.config = config;
    const ScreenPoint overlayPt = ResolveOverlayPoint(screenPt);

    if (!EnsureWindowForPoint(state, overlayPt)) {
        return;
    }

    const uint64_t nowMs = NowMs();
    state.lastInputMs = nowMs;
    const TrailPoint point{overlayPt, nowMs};
    if (!state.points.empty()) {
        const ScreenPoint& last = state.points.back().pt;
        const double dx = static_cast<double>(overlayPt.x - last.x);
        const double dy = static_cast<double>(overlayPt.y - last.y);
        if ((dx * dx + dy * dy) < 1.0) {
            return;
        }
    }
    state.points.push_back(point);

    const uint64_t duration = static_cast<uint64_t>(std::max(1, state.config.durationMs));
    while (!state.points.empty()) {
        const uint64_t age = nowMs - state.points.front().timeMs;
        if (age <= duration) {
            break;
        }
        state.points.pop_front();
    }

    RebuildPath(state, nowMs);
    EnsureTimer(state);
}

} // namespace
#endif

void UpdateLineTrail(const ScreenPoint& screenPt, const LineTrailConfig& config) {
#if !defined(__APPLE__)
    (void)screenPt;
    (void)config;
    return;
#else
    macos_overlay_support::RunOnMainThreadAsync(^{
      UpdateLineTrailOnMain(screenPt, config);
    });
#endif
}

void ResetLineTrail() {
#if !defined(__APPLE__)
    return;
#else
    macos_overlay_support::RunOnMainThreadAsync(^{
      ResetState(State());
    });
#endif
}

} // namespace mousefx::macos_line_trail
