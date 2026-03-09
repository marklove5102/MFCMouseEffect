#pragma once

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>

namespace mousefx::wasm {

struct GroupLocalOriginState final {
    float originXPx = 0.0f;
    float originYPx = 0.0f;
};

inline std::wstring BuildGroupLocalOriginKey(const std::wstring& activeManifestPath, uint32_t groupId) {
    if (activeManifestPath.empty() || groupId == 0u) {
        return {};
    }
    return activeManifestPath + L"#group-local-origin#" + std::to_wstring(groupId);
}

inline std::mutex& GroupLocalOriginMutex() {
    static std::mutex mutex;
    return mutex;
}

inline std::unordered_map<std::wstring, GroupLocalOriginState>& GroupLocalOriginEntries() {
    static std::unordered_map<std::wstring, GroupLocalOriginState> entries;
    return entries;
}

inline GroupLocalOriginState ResolveGroupLocalOrigin(
    const std::wstring& activeManifestPath,
    uint32_t groupId) {
    if (activeManifestPath.empty() || groupId == 0u) {
        return {};
    }

    std::lock_guard<std::mutex> lock(GroupLocalOriginMutex());
    const auto& entries = GroupLocalOriginEntries();
    const auto it = entries.find(BuildGroupLocalOriginKey(activeManifestPath, groupId));
    if (it == entries.end()) {
        return {};
    }
    return it->second;
}

inline void UpsertGroupLocalOrigin(
    const std::wstring& activeManifestPath,
    uint32_t groupId,
    float originXPx,
    float originYPx) {
    const std::wstring key = BuildGroupLocalOriginKey(activeManifestPath, groupId);
    if (key.empty()) {
        return;
    }

    std::lock_guard<std::mutex> lock(GroupLocalOriginMutex());
    GroupLocalOriginEntries()[key] = GroupLocalOriginState{
        originXPx,
        originYPx,
    };
}

inline void RemoveGroupLocalOrigin(
    const std::wstring& activeManifestPath,
    uint32_t groupId) {
    const std::wstring key = BuildGroupLocalOriginKey(activeManifestPath, groupId);
    if (key.empty()) {
        return;
    }

    std::lock_guard<std::mutex> lock(GroupLocalOriginMutex());
    GroupLocalOriginEntries().erase(key);
}

inline void ResetGroupLocalOriginsForManifest(const std::wstring& activeManifestPath) {
    if (activeManifestPath.empty()) {
        return;
    }

    std::lock_guard<std::mutex> lock(GroupLocalOriginMutex());
    auto& entries = GroupLocalOriginEntries();
    for (auto it = entries.begin(); it != entries.end();) {
        if (it->first.rfind(activeManifestPath + L"#group-local-origin#", 0) == 0) {
            it = entries.erase(it);
        } else {
            ++it;
        }
    }
}

inline void ResetAllGroupLocalOrigins() {
    std::lock_guard<std::mutex> lock(GroupLocalOriginMutex());
    GroupLocalOriginEntries().clear();
}

} // namespace mousefx::wasm
