#pragma once

#include "MouseFx/Core/Wasm/WasmPluginAbi.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>

namespace mousefx::wasm {

struct GroupMaterialState final {
    bool hasTintOverride = false;
    uint32_t tintArgb = 0xFFFFFFFFu;
    float intensityMultiplier = 1.0f;
    uint8_t styleKind = kGroupMaterialStyleNone;
    float styleAmount = 0.0f;
    float diffusionAmount = 0.0f;
    float persistenceAmount = 0.0f;
    float echoAmount = 0.0f;
    float echoDriftPx = 0.0f;
    uint8_t feedbackMode = kGroupMaterialFeedbackModeDirectional;
    float feedbackPhaseRad = 0.0f;
    uint8_t feedbackLayerCount = 1u;
    float feedbackLayerFalloff = 0.5f;
};

inline std::wstring BuildGroupMaterialKey(const std::wstring& activeManifestPath, uint32_t groupId) {
    if (activeManifestPath.empty() || groupId == 0u) {
        return {};
    }
    return activeManifestPath + L"#group-material#" + std::to_wstring(groupId);
}

inline std::mutex& GroupMaterialMutex() {
    static std::mutex mutex;
    return mutex;
}

inline std::unordered_map<std::wstring, GroupMaterialState>& GroupMaterialEntries() {
    static std::unordered_map<std::wstring, GroupMaterialState> entries;
    return entries;
}

inline float ClampGroupMaterialIntensity(float value) {
    return std::clamp(value, 0.0f, 2.0f);
}

inline float ClampGroupMaterialStyleAmount(float value) {
    return std::clamp(value, 0.0f, 1.0f);
}

inline float ClampGroupMaterialResponseAmount(float value) {
    return std::clamp(value, 0.0f, 1.0f);
}

inline float ClampGroupMaterialEchoAmount(float value) {
    return std::clamp(value, 0.0f, 1.0f);
}

inline float ClampGroupMaterialEchoDriftPx(float value) {
    return std::clamp(value, 0.0f, 96.0f);
}

inline uint8_t ClampGroupMaterialFeedbackLayerCount(uint8_t value) {
    return static_cast<uint8_t>(std::clamp<int>(static_cast<int>(value), 1, 4));
}

inline float ClampGroupMaterialFeedbackLayerFalloff(float value) {
    return std::clamp(value, 0.0f, 1.0f);
}

inline uint8_t ResolveGroupMaterialFeedbackMode(uint8_t value) {
    switch (value) {
    case kGroupMaterialFeedbackModeDirectional:
    case kGroupMaterialFeedbackModeTangential:
    case kGroupMaterialFeedbackModeSwirl:
        return value;
    default:
        return kGroupMaterialFeedbackModeDirectional;
    }
}

inline float NormalizeGroupMaterialPhaseRad(float value) {
    if (!std::isfinite(value)) {
        return 0.0f;
    }
    constexpr float kTau = 6.283185307179586f;
    constexpr float kPi = 3.141592653589793f;
    float normalized = std::fmod(value, kTau);
    if (normalized > kPi) {
        normalized -= kTau;
    } else if (normalized < -kPi) {
        normalized += kTau;
    }
    return normalized;
}

inline uint8_t ResolveGroupMaterialStyleKind(uint8_t value) {
    switch (value) {
    case kGroupMaterialStyleNone:
    case kGroupMaterialStyleSoftBloomLike:
    case kGroupMaterialStyleAfterimageLike:
        return value;
    default:
        return kGroupMaterialStyleNone;
    }
}

inline GroupMaterialState ResolveGroupMaterial(
    const std::wstring& activeManifestPath,
    uint32_t groupId) {
    if (activeManifestPath.empty() || groupId == 0u) {
        return {};
    }

    std::lock_guard<std::mutex> lock(GroupMaterialMutex());
    const auto& entries = GroupMaterialEntries();
    const auto it = entries.find(BuildGroupMaterialKey(activeManifestPath, groupId));
    if (it == entries.end()) {
        return {};
    }
    return it->second;
}

inline void UpsertGroupMaterial(
    const std::wstring& activeManifestPath,
    uint32_t groupId,
    bool hasTintOverride,
    uint32_t tintArgb,
    float intensityMultiplier,
    uint8_t styleKind,
    float styleAmount,
    float diffusionAmount,
    float persistenceAmount,
    float echoAmount,
    float echoDriftPx,
    uint8_t feedbackMode,
    float feedbackPhaseRad,
    uint8_t feedbackLayerCount,
    float feedbackLayerFalloff) {
    const std::wstring key = BuildGroupMaterialKey(activeManifestPath, groupId);
    if (key.empty()) {
        return;
    }

    std::lock_guard<std::mutex> lock(GroupMaterialMutex());
    GroupMaterialEntries()[key] = GroupMaterialState{
        hasTintOverride,
        tintArgb,
        ClampGroupMaterialIntensity(intensityMultiplier),
        ResolveGroupMaterialStyleKind(styleKind),
        ClampGroupMaterialStyleAmount(styleAmount),
        ClampGroupMaterialResponseAmount(diffusionAmount),
        ClampGroupMaterialResponseAmount(persistenceAmount),
        ClampGroupMaterialEchoAmount(echoAmount),
        ClampGroupMaterialEchoDriftPx(echoDriftPx),
        ResolveGroupMaterialFeedbackMode(feedbackMode),
        NormalizeGroupMaterialPhaseRad(feedbackPhaseRad),
        ClampGroupMaterialFeedbackLayerCount(feedbackLayerCount),
        ClampGroupMaterialFeedbackLayerFalloff(feedbackLayerFalloff),
    };
}

inline void RemoveGroupMaterial(
    const std::wstring& activeManifestPath,
    uint32_t groupId) {
    const std::wstring key = BuildGroupMaterialKey(activeManifestPath, groupId);
    if (key.empty()) {
        return;
    }

    std::lock_guard<std::mutex> lock(GroupMaterialMutex());
    GroupMaterialEntries().erase(key);
}

inline void ResetGroupMaterialsForManifest(const std::wstring& activeManifestPath) {
    if (activeManifestPath.empty()) {
        return;
    }

    std::lock_guard<std::mutex> lock(GroupMaterialMutex());
    auto& entries = GroupMaterialEntries();
    for (auto it = entries.begin(); it != entries.end();) {
        if (it->first.rfind(activeManifestPath + L"#group-material#", 0) == 0) {
            it = entries.erase(it);
        } else {
            ++it;
        }
    }
}

inline void ResetAllGroupMaterials() {
    std::lock_guard<std::mutex> lock(GroupMaterialMutex());
    GroupMaterialEntries().clear();
}

} // namespace mousefx::wasm
