#pragma once

#include <algorithm>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>

namespace mousefx::wasm {

struct GroupPresentationState final {
    float alphaMultiplier = 1.0f;
    bool visible = true;
};

inline std::wstring BuildGroupPresentationKey(const std::wstring& activeManifestPath, uint32_t groupId) {
    if (activeManifestPath.empty() || groupId == 0u) {
        return {};
    }
    return activeManifestPath + L"#group#" + std::to_wstring(groupId);
}

inline std::mutex& GroupPresentationMutex() {
    static std::mutex mutex;
    return mutex;
}

inline std::unordered_map<std::wstring, GroupPresentationState>& GroupPresentationEntries() {
    static std::unordered_map<std::wstring, GroupPresentationState> entries;
    return entries;
}

inline GroupPresentationState ResolveGroupPresentation(
    const std::wstring& activeManifestPath,
    uint32_t groupId) {
    if (activeManifestPath.empty() || groupId == 0u) {
        return {};
    }

    std::lock_guard<std::mutex> lock(GroupPresentationMutex());
    const auto& entries = GroupPresentationEntries();
    const auto it = entries.find(BuildGroupPresentationKey(activeManifestPath, groupId));
    if (it == entries.end()) {
        return {};
    }
    return it->second;
}

inline void UpsertGroupPresentation(
    const std::wstring& activeManifestPath,
    uint32_t groupId,
    float alphaMultiplier,
    bool visible) {
    const std::wstring key = BuildGroupPresentationKey(activeManifestPath, groupId);
    if (key.empty()) {
        return;
    }

    std::lock_guard<std::mutex> lock(GroupPresentationMutex());
    GroupPresentationEntries()[key] = GroupPresentationState{
        std::clamp(alphaMultiplier, 0.0f, 1.0f),
        visible,
    };
}

inline void RemoveGroupPresentation(
    const std::wstring& activeManifestPath,
    uint32_t groupId) {
    const std::wstring key = BuildGroupPresentationKey(activeManifestPath, groupId);
    if (key.empty()) {
        return;
    }

    std::lock_guard<std::mutex> lock(GroupPresentationMutex());
    GroupPresentationEntries().erase(key);
}

inline void ResetGroupPresentationsForManifest(const std::wstring& activeManifestPath) {
    if (activeManifestPath.empty()) {
        return;
    }

    std::lock_guard<std::mutex> lock(GroupPresentationMutex());
    auto& entries = GroupPresentationEntries();
    for (auto it = entries.begin(); it != entries.end();) {
        if (it->first.rfind(activeManifestPath + L"#group#", 0) == 0) {
            it = entries.erase(it);
        } else {
            ++it;
        }
    }
}

inline void ResetAllGroupPresentations() {
    std::lock_guard<std::mutex> lock(GroupPresentationMutex());
    GroupPresentationEntries().clear();
}

} // namespace mousefx::wasm
