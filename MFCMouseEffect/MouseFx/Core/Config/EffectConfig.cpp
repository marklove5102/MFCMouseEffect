#include "pch.h"
#include "EffectConfig.h"
#include "EffectConfigInternal.h"

#include "MouseFx/Utils/StringUtils.h"

namespace mousefx {
namespace {

bool ContainsTrailToken(const std::string& value, const char* token) {
    return value.find(token) != std::string::npos;
}

std::string NormalizeTrailTypeAlias(std::string type) {
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

} // namespace

Argb ArgbFromHex(const std::string& hex) {
    if (hex.empty() || hex[0] != '#') {
        return Argb{0};
    }

    std::string valueHex = hex.substr(1);
    uint32_t value = 0;

    try {
        if (valueHex.length() == 6) {
            value = 0xFF000000 | static_cast<uint32_t>(std::stoul(valueHex, nullptr, 16));
        } else if (valueHex.length() == 8) {
            value = static_cast<uint32_t>(std::stoul(valueHex, nullptr, 16));
        }
    } catch (...) {
        value = 0;
    }

    return Argb{value};
}

EffectConfig EffectConfig::GetDefault() {
    return EffectConfig{};
}

TrailHistoryProfile EffectConfig::GetTrailHistoryProfile(const std::string& type) const {
    const std::string normalized = NormalizeTrailTypeAlias(type);

    if (normalized == "electric") {
        return config_internal::SanitizeTrailHistoryProfile(trailProfiles.electric);
    }
    if (normalized == "streamer") {
        return config_internal::SanitizeTrailHistoryProfile(trailProfiles.streamer);
    }
    if (normalized == "meteor") {
        return config_internal::SanitizeTrailHistoryProfile(trailProfiles.meteor);
    }
    if (normalized == "tubes") {
        return config_internal::SanitizeTrailHistoryProfile(trailProfiles.tubes);
    }
    return config_internal::SanitizeTrailHistoryProfile(trailProfiles.line);
}

} // namespace mousefx
