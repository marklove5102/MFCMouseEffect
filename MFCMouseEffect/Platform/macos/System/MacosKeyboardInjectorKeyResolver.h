#pragma once

#include <cstdint>

namespace mousefx::macos_keyboard_injector {

struct ModifierMapping final {
    uint32_t vkCode;
    uint16_t macKeyCode;
    uint64_t flag;
};

bool ResolveModifierMapping(uint32_t vkCode, ModifierMapping* outMapping);
bool ResolveKeyCode(uint32_t vkCode, uint16_t* outKeyCode, uint64_t* outModifierFlag);

} // namespace mousefx::macos_keyboard_injector
