#pragma once

#include "MouseFx/Interfaces/RenderSemantics.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <mutex>
#include <string>
#include <unordered_map>

namespace mousefx::wasm {

struct GroupLayerState final {
    bool hasBlendOverride = false;
    mousefx::RenderBlendMode blendMode = mousefx::RenderBlendMode::Normal;
    int32_t sortBias = 0;
};

inline std::wstring BuildGroupLayerKey(const std::wstring& activeManifestPath, uint32_t groupId) {
    if (activeManifestPath.empty() || groupId == 0u) {
        return {};
    }
    return activeManifestPath + L"#group-layer#" + std::to_wstring(groupId);
}

inline std::mutex& GroupLayerMutex() {
    static std::mutex mutex;
    return mutex;
}

inline std::unordered_map<std::wstring, GroupLayerState>& GroupLayerEntries() {
    static std::unordered_map<std::wstring, GroupLayerState> entries;
    return entries;
}

inline GroupLayerState ResolveGroupLayer(
    const std::wstring& activeManifestPath,
    uint32_t groupId) {
    if (activeManifestPath.empty() || groupId == 0u) {
        return {};
    }

    std::lock_guard<std::mutex> lock(GroupLayerMutex());
    const auto& entries = GroupLayerEntries();
    const auto it = entries.find(BuildGroupLayerKey(activeManifestPath, groupId));
    if (it == entries.end()) {
        return {};
    }
    return it->second;
}

inline void UpsertGroupLayer(
    const std::wstring& activeManifestPath,
    uint32_t groupId,
    bool hasBlendOverride,
    mousefx::RenderBlendMode blendMode,
    int32_t sortBias) {
    const std::wstring key = BuildGroupLayerKey(activeManifestPath, groupId);
    if (key.empty()) {
        return;
    }

    std::lock_guard<std::mutex> lock(GroupLayerMutex());
    GroupLayerEntries()[key] = GroupLayerState{
        hasBlendOverride,
        blendMode,
        sortBias,
    };
}

inline void RemoveGroupLayer(
    const std::wstring& activeManifestPath,
    uint32_t groupId) {
    const std::wstring key = BuildGroupLayerKey(activeManifestPath, groupId);
    if (key.empty()) {
        return;
    }

    std::lock_guard<std::mutex> lock(GroupLayerMutex());
    GroupLayerEntries().erase(key);
}

inline void ResetGroupLayersForManifest(const std::wstring& activeManifestPath) {
    if (activeManifestPath.empty()) {
        return;
    }

    std::lock_guard<std::mutex> lock(GroupLayerMutex());
    auto& entries = GroupLayerEntries();
    for (auto it = entries.begin(); it != entries.end();) {
        if (it->first.rfind(activeManifestPath + L"#group-layer#", 0) == 0) {
            it = entries.erase(it);
        } else {
            ++it;
        }
    }
}

inline void ResetAllGroupLayers() {
    std::lock_guard<std::mutex> lock(GroupLayerMutex());
    GroupLayerEntries().clear();
}

inline mousefx::RenderBlendMode ResolveEffectiveGroupBlendMode(
    mousefx::RenderBlendMode baseBlendMode,
    const GroupLayerState& layerState) {
    return layerState.hasBlendOverride ? layerState.blendMode : baseBlendMode;
}

inline int32_t ResolveEffectiveGroupSortKey(
    int32_t baseSortKey,
    const GroupLayerState& layerState) {
    const int64_t combined = static_cast<int64_t>(baseSortKey) + static_cast<int64_t>(layerState.sortBias);
    return static_cast<int32_t>(std::clamp<int64_t>(
        combined,
        static_cast<int64_t>(std::numeric_limits<int32_t>::min()),
        static_cast<int64_t>(std::numeric_limits<int32_t>::max())));
}

} // namespace mousefx::wasm
