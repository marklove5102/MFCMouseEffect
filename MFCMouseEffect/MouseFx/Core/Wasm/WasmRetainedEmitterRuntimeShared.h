#pragma once

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace mousefx::wasm {

template <typename TEntry>
struct RetainedEmitterRuntimeStore final {
    using Entry = TEntry;

    static std::mutex& Mutex() {
        static std::mutex mutex;
        return mutex;
    }

    static std::unordered_map<std::wstring, Entry>& Entries() {
        static std::unordered_map<std::wstring, Entry> entries;
        return entries;
    }

    static std::atomic<uint64_t>& UpsertRequests() {
        static std::atomic<uint64_t> counter{0};
        return counter;
    }

    static std::atomic<uint64_t>& RemoveRequests() {
        static std::atomic<uint64_t> counter{0};
        return counter;
    }
};

inline std::wstring BuildRetainedEmitterKey(const std::wstring& activeManifestPath, uint32_t emitterId) {
    if (activeManifestPath.empty() || emitterId == 0u) {
        return {};
    }
    return activeManifestPath + L"#" + std::to_wstring(emitterId);
}

template <typename TStore, typename TIsActiveFn, typename TReleaseFn>
void PruneInactiveRetainedEmittersLocked(TIsActiveFn&& isActiveFn, TReleaseFn&& releaseFn) {
    auto& entries = TStore::Entries();
    for (auto it = entries.begin(); it != entries.end();) {
        if (isActiveFn(it->second)) {
            ++it;
            continue;
        }
        releaseFn(&it->second);
        it = entries.erase(it);
    }
}

template <typename TStore, typename TReleaseFn>
void ResetRetainedEmittersForManifest(
    const std::wstring& activeManifestPath,
    TReleaseFn&& releaseFn) {
    if (activeManifestPath.empty()) {
        return;
    }

    std::vector<typename TStore::Entry> entriesToRelease;
    {
        std::lock_guard<std::mutex> lock(TStore::Mutex());
        auto& entries = TStore::Entries();
        for (auto it = entries.begin(); it != entries.end();) {
            if (it->first.rfind(activeManifestPath + L"#", 0) == 0) {
                entriesToRelease.push_back(std::move(it->second));
                it = entries.erase(it);
            } else {
                ++it;
            }
        }
    }

    for (auto& entry : entriesToRelease) {
        releaseFn(&entry);
    }
}

template <typename TStore, typename TReleaseFn>
void ResetAllRetainedEmitters(TReleaseFn&& releaseFn) {
    std::vector<typename TStore::Entry> entriesToRelease;
    {
        std::lock_guard<std::mutex> lock(TStore::Mutex());
        auto& entries = TStore::Entries();
        entriesToRelease.reserve(entries.size());
        for (auto& entry : entries) {
            entriesToRelease.push_back(std::move(entry.second));
        }
        entries.clear();
    }

    for (auto& entry : entriesToRelease) {
        releaseFn(&entry);
    }
}

template <typename TStore, typename TMatchFn, typename TReleaseFn>
uint32_t RemoveRetainedEmittersForGroup(
    const std::wstring& activeManifestPath,
    uint32_t groupId,
    TMatchFn&& matchFn,
    TReleaseFn&& releaseFn) {
    if (activeManifestPath.empty() || groupId == 0u) {
        return 0u;
    }

    std::vector<typename TStore::Entry> entriesToRelease;
    {
        std::lock_guard<std::mutex> lock(TStore::Mutex());
        auto& entries = TStore::Entries();
        for (auto it = entries.begin(); it != entries.end();) {
            if (it->first.rfind(activeManifestPath + L"#", 0) == 0 && matchFn(it->second, groupId)) {
                entriesToRelease.push_back(std::move(it->second));
                it = entries.erase(it);
            } else {
                ++it;
            }
        }
    }

    for (auto& entry : entriesToRelease) {
        releaseFn(&entry);
    }
    return static_cast<uint32_t>(entriesToRelease.size());
}

template <typename TStore, typename TIsActiveFn, typename TReleaseFn>
uint64_t CountActiveRetainedEmitters(TIsActiveFn&& isActiveFn, TReleaseFn&& releaseFn) {
    std::lock_guard<std::mutex> lock(TStore::Mutex());
    PruneInactiveRetainedEmittersLocked<TStore>(
        std::forward<TIsActiveFn>(isActiveFn),
        std::forward<TReleaseFn>(releaseFn));
    return static_cast<uint64_t>(TStore::Entries().size());
}

} // namespace mousefx::wasm
