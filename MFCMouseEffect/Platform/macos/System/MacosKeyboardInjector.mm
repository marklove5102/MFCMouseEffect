#include "pch.h"

#include "Platform/macos/System/MacosKeyboardInjector.h"

#include "MouseFx/Core/Automation/KeyChord.h"
#include "MouseFx/Core/Protocol/VirtualKeyCodes.h"

#if defined(__APPLE__)
#import <ApplicationServices/ApplicationServices.h>
#import <Carbon/Carbon.h>
#endif

#include <cstdlib>
#include <array>
#include <vector>

namespace mousefx {
namespace {

struct ModifierMapping final {
    uint32_t vkCode;
    uint16_t macKeyCode;
    uint64_t flag;
};

bool IsKeyboardInjectorDryRunEnabled() {
    const char* raw = std::getenv("MFX_TEST_KEYBOARD_INJECTOR_DRY_RUN");
    if (raw == nullptr || raw[0] == '\0') {
        return false;
    }
    return raw[0] == '1' || raw[0] == 'y' || raw[0] == 'Y' || raw[0] == 't' || raw[0] == 'T';
}

bool ResolveModifierMapping(uint32_t vkCode, ModifierMapping* outMapping) {
#if !defined(__APPLE__)
    (void)vkCode;
    (void)outMapping;
    return false;
#else
    if (!outMapping) {
        return false;
    }

    switch (vkCode) {
    case vk::kShift:
    case vk::kLShift:
    case vk::kRShift:
        *outMapping = ModifierMapping{vkCode, static_cast<uint16_t>(kVK_Shift), kCGEventFlagMaskShift};
        return true;
    case vk::kControl:
    case vk::kLControl:
    case vk::kRControl:
        *outMapping = ModifierMapping{vkCode, static_cast<uint16_t>(kVK_Control), kCGEventFlagMaskControl};
        return true;
    case vk::kMenu:
    case vk::kLMenu:
    case vk::kRMenu:
        *outMapping = ModifierMapping{vkCode, static_cast<uint16_t>(kVK_Option), kCGEventFlagMaskAlternate};
        return true;
    case vk::kLWin:
    case vk::kRWin:
        *outMapping = ModifierMapping{vkCode, static_cast<uint16_t>(kVK_Command), kCGEventFlagMaskCommand};
        return true;
    default:
        break;
    }

    return false;
#endif
}

bool ResolvePrintableKeyCode(uint32_t vkCode, uint16_t* outKeyCode) {
#if !defined(__APPLE__)
    (void)vkCode;
    (void)outKeyCode;
    return false;
#else
    if (!outKeyCode) {
        return false;
    }

    if (vkCode >= 'A' && vkCode <= 'Z') {
        static constexpr std::array<uint16_t, 26> kLetterCodes{
            static_cast<uint16_t>(kVK_ANSI_A),
            static_cast<uint16_t>(kVK_ANSI_B),
            static_cast<uint16_t>(kVK_ANSI_C),
            static_cast<uint16_t>(kVK_ANSI_D),
            static_cast<uint16_t>(kVK_ANSI_E),
            static_cast<uint16_t>(kVK_ANSI_F),
            static_cast<uint16_t>(kVK_ANSI_G),
            static_cast<uint16_t>(kVK_ANSI_H),
            static_cast<uint16_t>(kVK_ANSI_I),
            static_cast<uint16_t>(kVK_ANSI_J),
            static_cast<uint16_t>(kVK_ANSI_K),
            static_cast<uint16_t>(kVK_ANSI_L),
            static_cast<uint16_t>(kVK_ANSI_M),
            static_cast<uint16_t>(kVK_ANSI_N),
            static_cast<uint16_t>(kVK_ANSI_O),
            static_cast<uint16_t>(kVK_ANSI_P),
            static_cast<uint16_t>(kVK_ANSI_Q),
            static_cast<uint16_t>(kVK_ANSI_R),
            static_cast<uint16_t>(kVK_ANSI_S),
            static_cast<uint16_t>(kVK_ANSI_T),
            static_cast<uint16_t>(kVK_ANSI_U),
            static_cast<uint16_t>(kVK_ANSI_V),
            static_cast<uint16_t>(kVK_ANSI_W),
            static_cast<uint16_t>(kVK_ANSI_X),
            static_cast<uint16_t>(kVK_ANSI_Y),
            static_cast<uint16_t>(kVK_ANSI_Z),
        };
        *outKeyCode = kLetterCodes[vkCode - 'A'];
        return true;
    }

    if (vkCode >= '0' && vkCode <= '9') {
        static constexpr std::array<uint16_t, 10> kDigitCodes{
            static_cast<uint16_t>(kVK_ANSI_0),
            static_cast<uint16_t>(kVK_ANSI_1),
            static_cast<uint16_t>(kVK_ANSI_2),
            static_cast<uint16_t>(kVK_ANSI_3),
            static_cast<uint16_t>(kVK_ANSI_4),
            static_cast<uint16_t>(kVK_ANSI_5),
            static_cast<uint16_t>(kVK_ANSI_6),
            static_cast<uint16_t>(kVK_ANSI_7),
            static_cast<uint16_t>(kVK_ANSI_8),
            static_cast<uint16_t>(kVK_ANSI_9),
        };
        *outKeyCode = kDigitCodes[vkCode - '0'];
        return true;
    }

    if (vkCode >= vk::kNumpad0 && vkCode <= vk::kNumpad9) {
        static constexpr std::array<uint16_t, 10> kNumpadCodes{
            static_cast<uint16_t>(kVK_ANSI_Keypad0),
            static_cast<uint16_t>(kVK_ANSI_Keypad1),
            static_cast<uint16_t>(kVK_ANSI_Keypad2),
            static_cast<uint16_t>(kVK_ANSI_Keypad3),
            static_cast<uint16_t>(kVK_ANSI_Keypad4),
            static_cast<uint16_t>(kVK_ANSI_Keypad5),
            static_cast<uint16_t>(kVK_ANSI_Keypad6),
            static_cast<uint16_t>(kVK_ANSI_Keypad7),
            static_cast<uint16_t>(kVK_ANSI_Keypad8),
            static_cast<uint16_t>(kVK_ANSI_Keypad9),
        };
        *outKeyCode = kNumpadCodes[vkCode - vk::kNumpad0];
        return true;
    }

    return false;
#endif
}

bool ResolveFunctionKeyCode(uint32_t vkCode, uint16_t* outKeyCode) {
#if !defined(__APPLE__)
    (void)vkCode;
    (void)outKeyCode;
    return false;
#else
    if (!outKeyCode || vkCode < vk::kF1 || vkCode > vk::kF24) {
        return false;
    }

    static constexpr std::array<uint16_t, 20> kFunctionCodes{
        static_cast<uint16_t>(kVK_F1),
        static_cast<uint16_t>(kVK_F2),
        static_cast<uint16_t>(kVK_F3),
        static_cast<uint16_t>(kVK_F4),
        static_cast<uint16_t>(kVK_F5),
        static_cast<uint16_t>(kVK_F6),
        static_cast<uint16_t>(kVK_F7),
        static_cast<uint16_t>(kVK_F8),
        static_cast<uint16_t>(kVK_F9),
        static_cast<uint16_t>(kVK_F10),
        static_cast<uint16_t>(kVK_F11),
        static_cast<uint16_t>(kVK_F12),
        static_cast<uint16_t>(kVK_F13),
        static_cast<uint16_t>(kVK_F14),
        static_cast<uint16_t>(kVK_F15),
        static_cast<uint16_t>(kVK_F16),
        static_cast<uint16_t>(kVK_F17),
        static_cast<uint16_t>(kVK_F18),
        static_cast<uint16_t>(kVK_F19),
        static_cast<uint16_t>(kVK_F20),
    };

    const size_t idx = static_cast<size_t>(vkCode - vk::kF1);
    if (idx >= kFunctionCodes.size()) {
        return false;
    }

    *outKeyCode = kFunctionCodes[idx];
    return true;
#endif
}

bool ResolveSpecialKeyCode(uint32_t vkCode, uint16_t* outKeyCode) {
#if !defined(__APPLE__)
    (void)vkCode;
    (void)outKeyCode;
    return false;
#else
    if (!outKeyCode) {
        return false;
    }

    switch (vkCode) {
    case vk::kBackspace: *outKeyCode = static_cast<uint16_t>(kVK_Delete); return true;
    case vk::kTab: *outKeyCode = static_cast<uint16_t>(kVK_Tab); return true;
    case vk::kReturn: *outKeyCode = static_cast<uint16_t>(kVK_Return); return true;
    case vk::kEscape: *outKeyCode = static_cast<uint16_t>(kVK_Escape); return true;
    case vk::kSpace: *outKeyCode = static_cast<uint16_t>(kVK_Space); return true;
    case vk::kPrior: *outKeyCode = static_cast<uint16_t>(kVK_PageUp); return true;
    case vk::kNext: *outKeyCode = static_cast<uint16_t>(kVK_PageDown); return true;
    case vk::kEnd: *outKeyCode = static_cast<uint16_t>(kVK_End); return true;
    case vk::kHome: *outKeyCode = static_cast<uint16_t>(kVK_Home); return true;
    case vk::kLeft: *outKeyCode = static_cast<uint16_t>(kVK_LeftArrow); return true;
    case vk::kUp: *outKeyCode = static_cast<uint16_t>(kVK_UpArrow); return true;
    case vk::kRight: *outKeyCode = static_cast<uint16_t>(kVK_RightArrow); return true;
    case vk::kDown: *outKeyCode = static_cast<uint16_t>(kVK_DownArrow); return true;
    case vk::kInsert: *outKeyCode = static_cast<uint16_t>(kVK_Help); return true;
    case vk::kDelete: *outKeyCode = static_cast<uint16_t>(kVK_ForwardDelete); return true;
    default: break;
    }

    return false;
#endif
}

bool ResolveKeyCode(uint32_t vkCode, uint16_t* outKeyCode, uint64_t* outModifierFlag) {
    if (!outKeyCode || !outModifierFlag) {
        return false;
    }

    *outModifierFlag = 0;

    ModifierMapping modifier{};
    if (ResolveModifierMapping(vkCode, &modifier)) {
        *outKeyCode = modifier.macKeyCode;
        *outModifierFlag = modifier.flag;
        return true;
    }

    if (ResolvePrintableKeyCode(vkCode, outKeyCode)) {
        return true;
    }
    if (ResolveFunctionKeyCode(vkCode, outKeyCode)) {
        return true;
    }
    if (ResolveSpecialKeyCode(vkCode, outKeyCode)) {
        return true;
    }

    return false;
}

bool PostKeyEvent(uint16_t keyCode, bool keyDown, uint64_t flags) {
#if !defined(__APPLE__)
    (void)keyCode;
    (void)keyDown;
    (void)flags;
    return false;
#else
    CGEventRef event = CGEventCreateKeyboardEvent(nullptr, static_cast<CGKeyCode>(keyCode), keyDown ? true : false);
    if (event == nullptr) {
        return false;
    }

    CGEventSetFlags(event, static_cast<CGEventFlags>(flags));
    CGEventPost(kCGHIDEventTap, event);
    CFRelease(event);
    return true;
#endif
}

} // namespace

bool MacosKeyboardInjector::SendChord(const std::string& chordText) {
#if !defined(__APPLE__)
    (void)chordText;
    return false;
#else
    KeyChord chord{};
    if (!ParseKeyChord(chordText, &chord)) {
        return false;
    }

    std::vector<ModifierMapping> modifiers;
    modifiers.reserve(chord.modifiers.size());
    for (uint32_t modifierVk : chord.modifiers) {
        ModifierMapping mapping{};
        if (!ResolveModifierMapping(modifierVk, &mapping)) {
            return false;
        }
        modifiers.push_back(mapping);
    }

    uint16_t keyCode = 0;
    uint64_t keyModifierFlag = 0;
    if (!ResolveKeyCode(chord.key, &keyCode, &keyModifierFlag)) {
        return false;
    }

    if (IsKeyboardInjectorDryRunEnabled()) {
        return true;
    }
    if (!AXIsProcessTrusted()) {
        return false;
    }

    uint64_t currentFlags = 0;
    for (const ModifierMapping& modifier : modifiers) {
        currentFlags |= modifier.flag;
        if (!PostKeyEvent(modifier.macKeyCode, true, currentFlags)) {
            return false;
        }
    }

    if (!PostKeyEvent(keyCode, true, currentFlags | keyModifierFlag)) {
        return false;
    }
    if (!PostKeyEvent(keyCode, false, currentFlags)) {
        return false;
    }

    for (auto it = modifiers.rbegin(); it != modifiers.rend(); ++it) {
        const uint64_t nextFlags = (currentFlags & ~(it->flag));
        if (!PostKeyEvent(it->macKeyCode, false, nextFlags)) {
            return false;
        }
        currentFlags = nextFlags;
    }

    return true;
#endif
}

} // namespace mousefx
