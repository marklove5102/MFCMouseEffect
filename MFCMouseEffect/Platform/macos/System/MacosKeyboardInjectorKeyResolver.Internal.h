#pragma once

#include <cstdint>

namespace mousefx::macos_keyboard_injector::resolver_detail {

bool ResolveNonModifierKeyCode(uint32_t vkCode, uint16_t* outKeyCode);

} // namespace mousefx::macos_keyboard_injector::resolver_detail
