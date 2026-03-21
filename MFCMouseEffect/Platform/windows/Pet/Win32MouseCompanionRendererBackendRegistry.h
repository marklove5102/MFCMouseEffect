#pragma once

#include "Platform/windows/Pet/IWin32MouseCompanionRendererBackend.h"

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace mousefx::windows {

class Win32MouseCompanionRendererBackendRegistry {
public:
    using Factory = std::function<std::unique_ptr<IWin32MouseCompanionRendererBackend>()>;
    struct Availability {
        bool available{true};
        std::string reason{};
        std::vector<std::string> unmetRequirements{};
    };
    using AvailabilityProbe = std::function<Availability()>;

    struct Descriptor {
        std::string name{};
        int priority = 0;
        bool available{true};
        std::string unavailableReason{};
        std::vector<std::string> unmetRequirements{};
    };

    static Win32MouseCompanionRendererBackendRegistry& Instance();

    void Register(
        const std::string& name,
        int priority,
        Factory factory,
        AvailabilityProbe availabilityProbe = {});
    std::unique_ptr<IWin32MouseCompanionRendererBackend> Create(const std::string& name) const;
    std::unique_ptr<IWin32MouseCompanionRendererBackend> CreateHighestPriority() const;
    std::vector<Descriptor> ListByPriority() const;

private:
    struct Entry {
        int priority = 0;
        uint64_t order = 0;
        Factory factory{};
        AvailabilityProbe availabilityProbe{};
    };

    std::map<std::string, Entry> entries_{};
    uint64_t nextOrder_ = 0;
};

template <typename T>
class Win32MouseCompanionRendererBackendRegistrar {
public:
    Win32MouseCompanionRendererBackendRegistrar(
        const std::string& name,
        int priority,
        Win32MouseCompanionRendererBackendRegistry::AvailabilityProbe availabilityProbe = {}) {
        Win32MouseCompanionRendererBackendRegistry::Instance().Register(
            name,
            priority,
            []() { return std::make_unique<T>(); },
            std::move(availabilityProbe));
    }
};

#define REGISTER_WIN32_MOUSE_COMPANION_RENDERER_BACKEND(Name, Priority, Class) \
    static mousefx::windows::Win32MouseCompanionRendererBackendRegistrar<Class> reg_##Class(Name, Priority);

} // namespace mousefx::windows
