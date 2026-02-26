#include "pch.h"

#include "Platform/macos/Effects/MacosTrailPulseOverlayStyle.h"
#include "MouseFx/Utils/StringUtils.h"

#if defined(__APPLE__)
#include <cmath>
#endif

namespace mousefx::macos_trail_pulse::detail {

#if defined(__APPLE__)
namespace {

bool ContainsTrailToken(const std::string& value, const char* token) {
    return value.find(token) != std::string::npos;
}

} // namespace

std::string NormalizeTrailType(const std::string& effectType) {
    const std::string value = ToLowerAscii(effectType);
    if (value.empty() || value == "none") {
        return "line";
    }
    if (ContainsTrailToken(value, "meteor")) {
        return "meteor";
    }
    if (ContainsTrailToken(value, "streamer") || ContainsTrailToken(value, "stream") || ContainsTrailToken(value, "neon")) {
        return "streamer";
    }
    if (ContainsTrailToken(value, "electric") || ContainsTrailToken(value, "arc")) {
        return "electric";
    }
    if (ContainsTrailToken(value, "tube") || ContainsTrailToken(value, "suspension")) {
        return "tubes";
    }
    if (ContainsTrailToken(value, "particle") || ContainsTrailToken(value, "spark")) {
        return "particle";
    }
    if (ContainsTrailToken(value, "line") || ContainsTrailToken(value, "default")) {
        return "line";
    }
    return "line";
}

NSColor* TrailStrokeColor(const std::string& trailType) {
    if (trailType == "meteor") {
        return [NSColor colorWithCalibratedRed:1.0 green:0.64 blue:0.30 alpha:0.95];
    }
    if (trailType == "streamer") {
        return [NSColor colorWithCalibratedRed:0.32 green:0.95 blue:0.92 alpha:0.95];
    }
    if (trailType == "electric") {
        return [NSColor colorWithCalibratedRed:0.58 green:0.73 blue:1.0 alpha:0.95];
    }
    if (trailType == "tubes") {
        return [NSColor colorWithCalibratedRed:0.43 green:0.88 blue:0.52 alpha:0.95];
    }
    if (trailType == "particle") {
        return [NSColor colorWithCalibratedRed:1.0 green:0.84 blue:0.34 alpha:0.95];
    }
    return [NSColor colorWithCalibratedRed:0.40 green:0.76 blue:1.0 alpha:0.95];
}

NSColor* TrailFillColor(const std::string& trailType) {
    NSColor* stroke = TrailStrokeColor(trailType);
    return [stroke colorWithAlphaComponent:0.24];
}

CGPathRef CreateTrailLinePath(CGRect bounds, double deltaX, double deltaY, const std::string& trailType) {
    const CGFloat cx = CGRectGetMidX(bounds);
    const CGFloat cy = CGRectGetMidY(bounds);
    const CGFloat len = (trailType == "particle") ? 10.0 : 20.0;
    const CGFloat width = (trailType == "particle") ? 2.0 : 3.0;

    CGFloat dx = static_cast<CGFloat>(deltaX);
    CGFloat dy = static_cast<CGFloat>(deltaY);
    const CGFloat norm = std::sqrt(dx * dx + dy * dy);
    if (norm < 0.0001) {
        dx = 1.0;
        dy = 0.0;
    } else {
        dx /= norm;
        dy /= norm;
    }

    CGMutablePathRef path = CGPathCreateMutable();
    CGPathMoveToPoint(path, nullptr, cx - dx * len, cy - dy * len);
    if (trailType == "electric") {
        CGPathAddLineToPoint(path, nullptr, cx - dy * width, cy + dx * width);
        CGPathAddLineToPoint(path, nullptr, cx + dy * width, cy - dx * width);
    }
    CGPathAddLineToPoint(path, nullptr, cx + dx * len, cy + dy * len);
    return path;
}
#endif

} // namespace mousefx::macos_trail_pulse::detail
