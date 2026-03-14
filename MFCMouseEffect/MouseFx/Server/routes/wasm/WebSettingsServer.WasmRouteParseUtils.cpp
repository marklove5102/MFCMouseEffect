#include "pch.h"
#include "WebSettingsServer.WasmRouteUtils.h"

#include <string>

#include "MouseFx/Core/Wasm/WasmPluginManifest.h"
#include "MouseFx/Utils/StringUtils.h"

using json = nlohmann::json;

namespace mousefx::websettings_wasm_routes {

json ParseObjectOrEmpty(const std::string& body) {
    if (body.empty()) {
        return json::object();
    }
    try {
        json parsed = json::parse(body);
        if (parsed.is_object()) {
            return parsed;
        }
    } catch (...) {
    }
    return json::object();
}

std::string ParseManifestPathUtf8(const json& payload) {
    if (!payload.contains("manifest_path") || !payload["manifest_path"].is_string()) {
        return {};
    }
    return TrimAscii(payload["manifest_path"].get<std::string>());
}

std::string ParseInitialPathUtf8(const json& payload) {
    if (!payload.contains("initial_path") || !payload["initial_path"].is_string()) {
        return {};
    }
    return TrimAscii(payload["initial_path"].get<std::string>());
}

namespace {

enum class ManifestSurfaceHint {
    Unknown,
    Indicator,
    Effects,
};

ManifestSurfaceHint InferSurfaceHintFromManifest(const wasm::PluginManifest& manifest) {
    constexpr uint32_t kIndicatorInputMask =
        wasm::kManifestInputKindIndicatorClickBit |
        wasm::kManifestInputKindIndicatorScrollBit |
        wasm::kManifestInputKindIndicatorKeyBit;
    constexpr uint32_t kEffectsInputMask = wasm::kManifestInputKindAllBits & ~kIndicatorInputMask;

    if (manifest.hasExplicitSurfaceKinds) {
        const bool hasIndicatorSurface = (manifest.surfaceKindsMask & wasm::kManifestSurfaceIndicatorBit) != 0u;
        const bool hasEffectsSurface = (manifest.surfaceKindsMask & wasm::kManifestSurfaceEffectsBit) != 0u;
        if (hasIndicatorSurface && !hasEffectsSurface) {
            return ManifestSurfaceHint::Indicator;
        }
        if (hasEffectsSurface && !hasIndicatorSurface) {
            return ManifestSurfaceHint::Effects;
        }
        return ManifestSurfaceHint::Unknown;
    }

    const bool hasIndicatorKinds = (manifest.inputKindsMask & kIndicatorInputMask) != 0u;
    const bool hasEffectsKinds = (manifest.inputKindsMask & kEffectsInputMask) != 0u;
    if (hasIndicatorKinds && !hasEffectsKinds) {
        return ManifestSurfaceHint::Indicator;
    }
    if (hasEffectsKinds && !hasIndicatorKinds) {
        return ManifestSurfaceHint::Effects;
    }
    return ManifestSurfaceHint::Unknown;
}

ManifestSurfaceHint InferSurfaceHintFromManifestPath(const std::string& manifestPathUtf8) {
    if (manifestPathUtf8.empty()) {
        return ManifestSurfaceHint::Unknown;
    }
    const wasm::PluginManifestLoadResult load =
        wasm::WasmPluginManifest::LoadFromFile(Utf8ToWString(manifestPathUtf8));
    if (!load.ok) {
        return ManifestSurfaceHint::Unknown;
    }
    return InferSurfaceHintFromManifest(load.manifest);
}

} // namespace

void ApplyManifestSurfaceHintIfMissing(std::string* surface, const std::string& manifestPathUtf8) {
    if (!surface || !surface->empty()) {
        return;
    }
    const ManifestSurfaceHint hint = InferSurfaceHintFromManifestPath(manifestPathUtf8);
    if (hint == ManifestSurfaceHint::Indicator) {
        *surface = "indicator";
    } else if (hint == ManifestSurfaceHint::Effects) {
        *surface = "effects";
    }
}

} // namespace mousefx::websettings_wasm_routes
