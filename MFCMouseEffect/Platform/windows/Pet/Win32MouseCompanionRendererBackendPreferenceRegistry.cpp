#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionRendererBackendPreferenceRegistry.h"

#include <algorithm>
#include <vector>

namespace mousefx::windows {

Win32MouseCompanionRendererBackendPreferenceRegistry&
Win32MouseCompanionRendererBackendPreferenceRegistry::Instance() {
    static Win32MouseCompanionRendererBackendPreferenceRegistry instance;
    return instance;
}

void Win32MouseCompanionRendererBackendPreferenceRegistry::Register(
    const std::string& name,
    int priority,
    Resolver resolver) {
    if (name.empty() || !resolver) {
        return;
    }

    const auto it = entries_.find(name);
    if (it == entries_.end()) {
        Entry entry{};
        entry.priority = priority;
        entry.order = nextOrder_++;
        entry.resolver = std::move(resolver);
        entries_.emplace(name, std::move(entry));
        return;
    }

    it->second.priority = priority;
    it->second.resolver = std::move(resolver);
}

Win32MouseCompanionRendererBackendPreferenceResolution
Win32MouseCompanionRendererBackendPreferenceRegistry::ResolveHighestPriority(
    const Win32MouseCompanionRendererBackendPreferenceRequest& request) const {
    struct RankedEntry {
        std::string name{};
        int priority = 0;
        uint64_t order = 0;
        Resolver resolver{};
    };

    std::vector<RankedEntry> ranked;
    ranked.reserve(entries_.size());
    for (const auto& kv : entries_) {
        RankedEntry entry{};
        entry.name = kv.first;
        entry.priority = kv.second.priority;
        entry.order = kv.second.order;
        entry.resolver = kv.second.resolver;
        ranked.push_back(std::move(entry));
    }

    std::sort(ranked.begin(), ranked.end(), [](const RankedEntry& a, const RankedEntry& b) {
        if (a.priority != b.priority) {
            return a.priority > b.priority;
        }
        return a.order < b.order;
    });

    for (const auto& entry : ranked) {
        if (!entry.resolver) {
            continue;
        }
        const auto resolution = entry.resolver(request);
        if (resolution.matched) {
            return resolution;
        }
    }

    return {};
}

} // namespace mousefx::windows
