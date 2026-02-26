#include "pch.h"

#include "Platform/macos/Effects/MacosEffectCreatorRegistry.h"
#include "Platform/macos/Effects/MacosEffectCreatorRegistry.Internal.h"

namespace mousefx::macos_effect_registry {

std::unique_ptr<IMouseEffect> Create(EffectCategory category, const std::string& type, const EffectConfig& config) {
    const size_t idx = detail::CategoryIndex(category);
    const auto& registryTable = detail::RegistryTable();
    if (idx >= registryTable.size()) {
        return nullptr;
    }

    const auto& categoryRegistry = registryTable[idx];
    if (const auto it = categoryRegistry.typedCreators.find(type); it != categoryRegistry.typedCreators.end()) {
        return it->second(type, config);
    }
    if (categoryRegistry.fallbackCreator != nullptr) {
        return categoryRegistry.fallbackCreator(type, config);
    }
    return nullptr;
}

} // namespace mousefx::macos_effect_registry
