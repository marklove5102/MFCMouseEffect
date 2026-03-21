#pragma once

#include "Platform/windows/Pet/Win32MouseCompanionRendererBackendPreference.h"

#include <cstdint>
#include <functional>
#include <map>
#include <string>

namespace mousefx::windows {

struct Win32MouseCompanionRendererBackendPreferenceResolution {
    bool matched = false;
    Win32MouseCompanionRendererBackendPreference preference{};
};

class Win32MouseCompanionRendererBackendPreferenceRegistry {
public:
    using Resolver =
        std::function<Win32MouseCompanionRendererBackendPreferenceResolution(
            const Win32MouseCompanionRendererBackendPreferenceRequest&)>;

    static Win32MouseCompanionRendererBackendPreferenceRegistry& Instance();

    void Register(const std::string& name, int priority, Resolver resolver);
    Win32MouseCompanionRendererBackendPreferenceResolution ResolveHighestPriority(
        const Win32MouseCompanionRendererBackendPreferenceRequest& request) const;

private:
    struct Entry {
        int priority = 0;
        uint64_t order = 0;
        Resolver resolver{};
    };

    std::map<std::string, Entry> entries_{};
    uint64_t nextOrder_ = 0;
};

} // namespace mousefx::windows
