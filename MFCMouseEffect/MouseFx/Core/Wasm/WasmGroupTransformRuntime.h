#pragma once

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>

namespace mousefx::wasm {

struct GroupTransformState final {
    float offsetXPx = 0.0f;
    float offsetYPx = 0.0f;
    float rotationRad = 0.0f;
    float uniformScale = 1.0f;
    float pivotXPx = 0.0f;
    float pivotYPx = 0.0f;
    float scaleX = 1.0f;
    float scaleY = 1.0f;
};

inline std::wstring BuildGroupTransformKey(const std::wstring& activeManifestPath, uint32_t groupId) {
    if (activeManifestPath.empty() || groupId == 0u) {
        return {};
    }
    return activeManifestPath + L"#group-transform#" + std::to_wstring(groupId);
}

inline std::mutex& GroupTransformMutex() {
    static std::mutex mutex;
    return mutex;
}

inline std::unordered_map<std::wstring, GroupTransformState>& GroupTransformEntries() {
    static std::unordered_map<std::wstring, GroupTransformState> entries;
    return entries;
}

inline GroupTransformState ResolveGroupTransform(
    const std::wstring& activeManifestPath,
    uint32_t groupId) {
    if (activeManifestPath.empty() || groupId == 0u) {
        return {};
    }

    std::lock_guard<std::mutex> lock(GroupTransformMutex());
    const auto& entries = GroupTransformEntries();
    const auto it = entries.find(BuildGroupTransformKey(activeManifestPath, groupId));
    if (it == entries.end()) {
        return {};
    }
    return it->second;
}

inline void UpsertGroupTransform(
    const std::wstring& activeManifestPath,
    uint32_t groupId,
    float offsetXPx,
    float offsetYPx,
    float rotationRad = 0.0f,
    float uniformScale = 1.0f,
    float pivotXPx = 0.0f,
    float pivotYPx = 0.0f,
    float scaleX = 1.0f,
    float scaleY = 1.0f) {
    const std::wstring key = BuildGroupTransformKey(activeManifestPath, groupId);
    if (key.empty()) {
        return;
    }

    std::lock_guard<std::mutex> lock(GroupTransformMutex());
    GroupTransformEntries()[key] = GroupTransformState{
        offsetXPx,
        offsetYPx,
        rotationRad,
        uniformScale,
        pivotXPx,
        pivotYPx,
        scaleX,
        scaleY,
    };
}

inline void RemoveGroupTransform(
    const std::wstring& activeManifestPath,
    uint32_t groupId) {
    const std::wstring key = BuildGroupTransformKey(activeManifestPath, groupId);
    if (key.empty()) {
        return;
    }

    std::lock_guard<std::mutex> lock(GroupTransformMutex());
    GroupTransformEntries().erase(key);
}

inline void ResetGroupTransformsForManifest(const std::wstring& activeManifestPath) {
    if (activeManifestPath.empty()) {
        return;
    }

    std::lock_guard<std::mutex> lock(GroupTransformMutex());
    auto& entries = GroupTransformEntries();
    for (auto it = entries.begin(); it != entries.end();) {
        if (it->first.rfind(activeManifestPath + L"#group-transform#", 0) == 0) {
            it = entries.erase(it);
        } else {
            ++it;
        }
    }
}

inline void ResetAllGroupTransforms() {
    std::lock_guard<std::mutex> lock(GroupTransformMutex());
    GroupTransformEntries().clear();
}

} // namespace mousefx::wasm
