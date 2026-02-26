#pragma once

#include "Platform/macos/Effects/MacosEffectRenderProfile.h"

#include "MouseFx/Utils/StringUtils.h"

namespace mousefx::macos_effect_profile::detail {

inline double ClampDouble(double value, double lo, double hi) {
    if (value < lo) {
        return lo;
    }
    if (value > hi) {
        return hi;
    }
    return value;
}

inline bool ContainsTrailToken(const std::string& value, const char* token) {
    return value.find(token) != std::string::npos;
}

inline std::string NormalizeTrailTypeAlias(std::string type) {
    type = ToLowerAscii(type);
    if (type == "scifi" || type == "sci-fi" || type == "sci_fi") {
        return "tubes";
    }
    if (ContainsTrailToken(type, "meteor")) {
        return "meteor";
    }
    if (ContainsTrailToken(type, "streamer") || ContainsTrailToken(type, "stream") || ContainsTrailToken(type, "neon")) {
        return "streamer";
    }
    if (ContainsTrailToken(type, "electric") || ContainsTrailToken(type, "arc")) {
        return "electric";
    }
    if (ContainsTrailToken(type, "tube") || ContainsTrailToken(type, "suspension")) {
        return "tubes";
    }
    if (ContainsTrailToken(type, "line") || ContainsTrailToken(type, "default")) {
        return "line";
    }
    if (type == "particle" || ContainsTrailToken(type, "spark")) {
        return "particle";
    }
    return type;
}

inline TrailThrottleProfile ResolveTrailThrottleProfileByType(const std::string& trailType) {
    const std::string type = NormalizeTrailTypeAlias(trailType);
    if (type == "particle") {
        return {10, 3.0};
    }
    if (type == "meteor") {
        return {14, 5.0};
    }
    if (type == "streamer") {
        return {12, 4.0};
    }
    if (type == "electric") {
        return {15, 6.0};
    }
    if (type == "tubes") {
        return {18, 8.0};
    }
    return {};
}

} // namespace mousefx::macos_effect_profile::detail
