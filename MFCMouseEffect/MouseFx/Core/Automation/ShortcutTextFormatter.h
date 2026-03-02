#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "MouseFx/Core/Protocol/InputTypes.h"
#include "MouseFx/Core/Protocol/VirtualKeyCodes.h"

namespace mousefx::shortcut_text {

inline bool IsModifierVirtualKey(uint32_t vkCode) {
    switch (vkCode) {
    case vk::kControl:
    case vk::kLControl:
    case vk::kRControl:
    case vk::kShift:
    case vk::kLShift:
    case vk::kRShift:
    case vk::kMenu:
    case vk::kLMenu:
    case vk::kRMenu:
    case vk::kLWin:
    case vk::kRWin:
        return true;
    default:
        return false;
    }
}

inline std::string KeyTokenFromVirtualKey(uint32_t vkCode) {
    if (vkCode >= 'A' && vkCode <= 'Z') {
        return std::string(1, static_cast<char>(vkCode));
    }
    if (vkCode >= '0' && vkCode <= '9') {
        return std::string(1, static_cast<char>(vkCode));
    }
    if (vkCode >= vk::kF1 && vkCode <= vk::kF24) {
        const int index = static_cast<int>(vkCode - vk::kF1 + 1);
        return "F" + std::to_string(index);
    }
    if (vkCode >= vk::kNumpad0 && vkCode <= vk::kNumpad9) {
        const int index = static_cast<int>(vkCode - vk::kNumpad0);
        return std::to_string(index);
    }

    switch (vkCode) {
    case vk::kTab: return "Tab";
    case vk::kReturn: return "Enter";
    case vk::kEscape: return "Esc";
    case vk::kSpace: return "Space";
    case vk::kBackspace: return "Backspace";
    case vk::kDelete: return "Delete";
    case vk::kInsert: return "Insert";
    case vk::kHome: return "Home";
    case vk::kEnd: return "End";
    case vk::kPrior: return "PageUp";
    case vk::kNext: return "PageDown";
    case vk::kUp: return "Up";
    case vk::kDown: return "Down";
    case vk::kLeft: return "Left";
    case vk::kRight: return "Right";
    case vk::kCapital: return "CapsLock";
    case vk::kSnapshot: return "PrintScreen";
    case vk::kPause: return "Pause";
    case vk::kApps: return "Apps";
    default:
        break;
    }

    return {};
}

inline std::string JoinTokens(const std::vector<std::string>& tokens) {
    std::string out;
    for (const std::string& token : tokens) {
        if (token.empty()) {
            continue;
        }
        if (!out.empty()) {
            out.push_back('+');
        }
        out += token;
    }
    return out;
}

inline std::string FormatShortcutText(const KeyEvent& ev) {
    if (IsModifierVirtualKey(ev.vkCode)) {
        return {};
    }

    const std::string keyToken = KeyTokenFromVirtualKey(ev.vkCode);
    if (keyToken.empty()) {
        return {};
    }

    std::vector<std::string> tokens;
    if (ev.ctrl) tokens.emplace_back("Ctrl");
    if (ev.shift) tokens.emplace_back("Shift");
    if (ev.alt) tokens.emplace_back("Alt");
    if (ev.win) tokens.emplace_back("Win");
    tokens.push_back(keyToken);
    return JoinTokens(tokens);
}

} // namespace mousefx::shortcut_text
