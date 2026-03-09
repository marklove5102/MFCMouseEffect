#pragma once

#include "MouseFx/Core/Wasm/WasmPluginAbi.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>

namespace mousefx::wasm {

struct GroupPassStageState final {
    uint8_t passKind = kGroupPassKindNone;
    float passAmount = 0.0f;
    float responseAmount = 0.0f;
    uint8_t blendMode = kGroupPassBlendModeMultiply;
    float blendWeight = 1.0f;
    uint8_t routeMask = kGroupPassRouteAll;
    float glowResponseMultiplier = 1.0f;
    float spriteResponseMultiplier = 1.0f;
    float particleResponseMultiplier = 1.0f;
    float ribbonResponseMultiplier = 1.0f;
    float quadResponseMultiplier = 1.0f;
    float phaseRateRadPerSec = 0.0f;
    float decayPerSec = 0.0f;
    float decayFloor = 1.0f;
    uint8_t temporalMode = kGroupPassTemporalModeExponential;
    float temporalStrength = 1.0f;
    uint8_t feedbackLayerCount = 1u;
    float feedbackLayerFalloff = 0.5f;
};

struct GroupPassState final {
    uint8_t passKind = kGroupPassKindNone;
    float passAmount = 0.0f;
    float responseAmount = 0.0f;
    GroupPassStageState secondaryStage{};
    GroupPassStageState tertiaryStage{};
    int64_t updatedAtMonotonicMs = 0;
    uint8_t passMode = kGroupPassModeDirectional;
    float phaseRad = 0.0f;
    uint8_t feedbackLayerCount = 1u;
    float feedbackLayerFalloff = 0.5f;
};

inline std::wstring BuildGroupPassKey(const std::wstring& activeManifestPath, uint32_t groupId) {
    if (activeManifestPath.empty() || groupId == 0u) {
        return {};
    }
    return activeManifestPath + L"#group-pass#" + std::to_wstring(groupId);
}

inline std::mutex& GroupPassMutex() {
    static std::mutex mutex;
    return mutex;
}

inline std::unordered_map<std::wstring, GroupPassState>& GroupPassEntries() {
    static std::unordered_map<std::wstring, GroupPassState> entries;
    return entries;
}

inline uint8_t ResolveGroupPassKind(uint8_t value) {
    switch (value) {
    case kGroupPassKindNone:
    case kGroupPassKindSoftBloomLike:
    case kGroupPassKindAfterimageLike:
    case kGroupPassKindEchoLike:
        return value;
    default:
        return kGroupPassKindNone;
    }
}

inline float ClampGroupPassAmount(float value) {
    return std::clamp(value, 0.0f, 1.0f);
}

inline float ClampGroupPassResponseAmount(float value) {
    return std::clamp(value, 0.0f, 1.0f);
}

inline uint8_t ResolveGroupPassBlendMode(uint8_t value) {
    switch (value) {
    case kGroupPassBlendModeMultiply:
    case kGroupPassBlendModeLerp:
        return value;
    default:
        return kGroupPassBlendModeMultiply;
    }
}

inline float ClampGroupPassBlendWeight(float value) {
    return std::clamp(value, 0.0f, 1.0f);
}

inline uint8_t ResolveGroupPassRouteMask(uint8_t value) {
    const uint8_t resolved = static_cast<uint8_t>(value & kGroupPassRouteAll);
    return resolved == 0u ? kGroupPassRouteAll : resolved;
}

inline float ClampGroupPassLaneResponseMultiplier(float value) {
    return std::clamp(value, 0.0f, 2.0f);
}

inline float ClampGroupPassPhaseRateRadPerSec(float value) {
    return std::clamp(value, -12.5663706f, 12.5663706f);
}

inline float ClampGroupPassDecayPerSec(float value) {
    return std::clamp(value, 0.0f, 8.0f);
}

inline float ClampGroupPassDecayFloor(float value) {
    return std::clamp(value, 0.0f, 1.0f);
}

inline uint8_t ResolveGroupPassTemporalMode(uint8_t value) {
    switch (value) {
    case kGroupPassTemporalModeExponential:
    case kGroupPassTemporalModeLinear:
    case kGroupPassTemporalModePulse:
        return value;
    default:
        return kGroupPassTemporalModeExponential;
    }
}

inline float ClampGroupPassTemporalStrength(float value) {
    return std::clamp(value, 0.0f, 1.0f);
}

inline int64_t CurrentGroupPassMonotonicMs() {
    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
}

inline float ResolveGroupPassElapsedSec(const GroupPassState& passState) {
    if (passState.updatedAtMonotonicMs <= 0) {
        return 0.0f;
    }
    const int64_t elapsedMs = std::max<int64_t>(0, CurrentGroupPassMonotonicMs() - passState.updatedAtMonotonicMs);
    return static_cast<float>(elapsedMs) / 1000.0f;
}

inline uint8_t ResolveGroupPassMode(uint8_t value) {
    switch (value) {
    case kGroupPassModeDirectional:
    case kGroupPassModeTangential:
    case kGroupPassModeSwirl:
        return value;
    default:
        return kGroupPassModeDirectional;
    }
}

inline float NormalizeGroupPassPhaseRad(float value) {
    constexpr float kTwoPi = 6.28318530717958647692f;
    if (!std::isfinite(value)) {
        return 0.0f;
    }
    float normalized = std::fmod(value, kTwoPi);
    if (normalized < 0.0f) {
        normalized += kTwoPi;
    }
    return normalized;
}

inline uint8_t ClampGroupPassFeedbackLayerCount(uint8_t value) {
    return static_cast<uint8_t>(std::clamp<int>(static_cast<int>(value), 1, 4));
}

inline float ClampGroupPassFeedbackLayerFalloff(float value) {
    return std::clamp(value, 0.0f, 1.0f);
}

inline GroupPassStageState NormalizeGroupPassStageState(const GroupPassStageState& stageState) {
    return GroupPassStageState{
        ResolveGroupPassKind(stageState.passKind),
        ClampGroupPassAmount(stageState.passAmount),
        ClampGroupPassResponseAmount(stageState.responseAmount),
        ResolveGroupPassBlendMode(stageState.blendMode),
        ClampGroupPassBlendWeight(stageState.blendWeight),
        ResolveGroupPassRouteMask(stageState.routeMask),
        ClampGroupPassLaneResponseMultiplier(stageState.glowResponseMultiplier),
        ClampGroupPassLaneResponseMultiplier(stageState.spriteResponseMultiplier),
        ClampGroupPassLaneResponseMultiplier(stageState.particleResponseMultiplier),
        ClampGroupPassLaneResponseMultiplier(stageState.ribbonResponseMultiplier),
        ClampGroupPassLaneResponseMultiplier(stageState.quadResponseMultiplier),
        ClampGroupPassPhaseRateRadPerSec(stageState.phaseRateRadPerSec),
        ClampGroupPassDecayPerSec(stageState.decayPerSec),
        ClampGroupPassDecayFloor(stageState.decayFloor),
        ResolveGroupPassTemporalMode(stageState.temporalMode),
        ClampGroupPassTemporalStrength(stageState.temporalStrength),
        ClampGroupPassFeedbackLayerCount(stageState.feedbackLayerCount),
        ClampGroupPassFeedbackLayerFalloff(stageState.feedbackLayerFalloff),
    };
}

inline GroupPassState ResolveGroupPass(
    const std::wstring& activeManifestPath,
    uint32_t groupId) {
    if (activeManifestPath.empty() || groupId == 0u) {
        return {};
    }

    std::lock_guard<std::mutex> lock(GroupPassMutex());
    const auto& entries = GroupPassEntries();
    const auto it = entries.find(BuildGroupPassKey(activeManifestPath, groupId));
    if (it == entries.end()) {
        return {};
    }
    return it->second;
}

inline void UpsertGroupPass(
    const std::wstring& activeManifestPath,
    uint32_t groupId,
    uint8_t passKind,
    float passAmount,
    float responseAmount,
    const GroupPassStageState& secondaryStage = {},
    const GroupPassStageState& tertiaryStage = {},
    uint8_t passMode = kGroupPassModeDirectional,
    float phaseRad = 0.0f,
    uint8_t feedbackLayerCount = 1u,
    float feedbackLayerFalloff = 0.5f) {
    const std::wstring key = BuildGroupPassKey(activeManifestPath, groupId);
    if (key.empty()) {
        return;
    }

    std::lock_guard<std::mutex> lock(GroupPassMutex());
    GroupPassEntries()[key] = GroupPassState{
        ResolveGroupPassKind(passKind),
        ClampGroupPassAmount(passAmount),
        ClampGroupPassResponseAmount(responseAmount),
        NormalizeGroupPassStageState(secondaryStage),
        NormalizeGroupPassStageState(tertiaryStage),
        CurrentGroupPassMonotonicMs(),
        ResolveGroupPassMode(passMode),
        NormalizeGroupPassPhaseRad(phaseRad),
        ClampGroupPassFeedbackLayerCount(feedbackLayerCount),
        ClampGroupPassFeedbackLayerFalloff(feedbackLayerFalloff),
    };
}

inline void RemoveGroupPass(
    const std::wstring& activeManifestPath,
    uint32_t groupId) {
    const std::wstring key = BuildGroupPassKey(activeManifestPath, groupId);
    if (key.empty()) {
        return;
    }

    std::lock_guard<std::mutex> lock(GroupPassMutex());
    GroupPassEntries().erase(key);
}

inline void ResetGroupPassesForManifest(const std::wstring& activeManifestPath) {
    if (activeManifestPath.empty()) {
        return;
    }

    std::lock_guard<std::mutex> lock(GroupPassMutex());
    auto& entries = GroupPassEntries();
    for (auto it = entries.begin(); it != entries.end();) {
        if (it->first.rfind(activeManifestPath + L"#group-pass#", 0) == 0) {
            it = entries.erase(it);
        } else {
            ++it;
        }
    }
}

inline void ResetAllGroupPasses() {
    std::lock_guard<std::mutex> lock(GroupPassMutex());
    GroupPassEntries().clear();
}

} // namespace mousefx::wasm
