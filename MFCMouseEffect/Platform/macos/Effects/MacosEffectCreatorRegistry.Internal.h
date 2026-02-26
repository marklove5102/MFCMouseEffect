#pragma once

#include "Platform/macos/Effects/MacosEffectCreatorRegistry.h"

#include <array>
#include <unordered_map>

namespace mousefx::macos_effect_registry::detail {

using EffectCreator = std::unique_ptr<IMouseEffect> (*)(const std::string& type, const EffectConfig& config);

struct CategoryRegistryEntry {
    std::unordered_map<std::string, EffectCreator> typedCreators{};
    EffectCreator fallbackCreator = nullptr;
};

constexpr size_t CategoryIndex(EffectCategory category) {
    return static_cast<size_t>(category);
}

const std::array<CategoryRegistryEntry, CategoryIndex(EffectCategory::Count)>& RegistryTable();

} // namespace mousefx::macos_effect_registry::detail
