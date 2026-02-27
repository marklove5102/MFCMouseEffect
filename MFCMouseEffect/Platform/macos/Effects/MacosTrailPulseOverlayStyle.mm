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

NSColor* ArgbToNsColor(uint32_t argb) {
    const CGFloat alpha = static_cast<CGFloat>((argb >> 24) & 0xFFu) / 255.0;
    const CGFloat red = static_cast<CGFloat>((argb >> 16) & 0xFFu) / 255.0;
    const CGFloat green = static_cast<CGFloat>((argb >> 8) & 0xFFu) / 255.0;
    const CGFloat blue = static_cast<CGFloat>(argb & 0xFFu) / 255.0;
    return [NSColor colorWithCalibratedRed:red green:green blue:blue alpha:alpha];
}

const macos_effect_profile::TrailRenderProfile::TypeColorProfile& ResolveTrailTypeColor(
    const std::string& trailType,
    const macos_effect_profile::TrailRenderProfile& profile) {
    if (trailType == "meteor") return profile.meteor;
    if (trailType == "streamer") return profile.streamer;
    if (trailType == "electric") return profile.electric;
    if (trailType == "tubes") return profile.tubes;
    if (trailType == "particle") return profile.particle;
    return profile.line;
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

NSColor* TrailStrokeColor(
    const std::string& trailType,
    const macos_effect_profile::TrailRenderProfile& profile) {
    return ArgbToNsColor(ResolveTrailTypeColor(trailType, profile).strokeArgb);
}

NSColor* TrailFillColor(
    const std::string& trailType,
    const macos_effect_profile::TrailRenderProfile& profile) {
    return ArgbToNsColor(ResolveTrailTypeColor(trailType, profile).fillArgb);
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
