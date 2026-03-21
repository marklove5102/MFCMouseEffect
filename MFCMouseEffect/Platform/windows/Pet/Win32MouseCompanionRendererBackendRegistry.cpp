#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionRendererBackendRegistry.h"

#include <algorithm>

namespace mousefx::windows {

Win32MouseCompanionRendererBackendRegistry& Win32MouseCompanionRendererBackendRegistry::Instance() {
    static Win32MouseCompanionRendererBackendRegistry instance;
    return instance;
}

void Win32MouseCompanionRendererBackendRegistry::Register(
    const std::string& name,
    int priority,
    Factory factory) {
    if (name.empty() || !factory) {
        return;
    }

    const auto it = entries_.find(name);
    if (it == entries_.end()) {
        Entry entry{};
        entry.priority = priority;
        entry.order = nextOrder_++;
        entry.factory = std::move(factory);
        entries_.emplace(name, std::move(entry));
        return;
    }

    it->second.priority = priority;
    it->second.factory = std::move(factory);
}

std::unique_ptr<IWin32MouseCompanionRendererBackend> Win32MouseCompanionRendererBackendRegistry::Create(
    const std::string& name) const {
    const auto it = entries_.find(name);
    if (it == entries_.end() || !it->second.factory) {
        return nullptr;
    }
    return it->second.factory();
}

std::unique_ptr<IWin32MouseCompanionRendererBackend>
Win32MouseCompanionRendererBackendRegistry::CreateHighestPriority() const {
    const auto descriptors = ListByPriority();
    for (const auto& descriptor : descriptors) {
        auto backend = Create(descriptor.name);
        if (backend) {
            return backend;
        }
    }
    return nullptr;
}

std::vector<Win32MouseCompanionRendererBackendRegistry::Descriptor>
Win32MouseCompanionRendererBackendRegistry::ListByPriority() const {
    struct RankedDescriptor {
        Descriptor descriptor{};
        uint64_t order = 0;
    };

    std::vector<RankedDescriptor> ranked;
    ranked.reserve(entries_.size());
    for (const auto& kv : entries_) {
        RankedDescriptor item{};
        item.descriptor.name = kv.first;
        item.descriptor.priority = kv.second.priority;
        item.order = kv.second.order;
        ranked.push_back(std::move(item));
    }

    std::sort(ranked.begin(), ranked.end(), [](const RankedDescriptor& a, const RankedDescriptor& b) {
        if (a.descriptor.priority != b.descriptor.priority) {
            return a.descriptor.priority > b.descriptor.priority;
        }
        return a.order < b.order;
    });

    std::vector<Descriptor> output;
    output.reserve(ranked.size());
    for (const auto& item : ranked) {
        output.push_back(item.descriptor);
    }
    return output;
}

} // namespace mousefx::windows
