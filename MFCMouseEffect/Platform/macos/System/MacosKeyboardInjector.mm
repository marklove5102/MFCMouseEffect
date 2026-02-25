#include "pch.h"

#include "Platform/macos/System/MacosKeyboardInjector.h"

#include "MouseFx/Core/Automation/KeyChord.h"
#include "Platform/macos/System/MacosKeyboardInjectorKeyResolver.h"

#if defined(__APPLE__)
#import <ApplicationServices/ApplicationServices.h>
#endif

#include <cstdlib>
#include <vector>

namespace mousefx {
namespace {

bool IsKeyboardInjectorDryRunEnabled() {
    const char* raw = std::getenv("MFX_TEST_KEYBOARD_INJECTOR_DRY_RUN");
    if (raw == nullptr || raw[0] == '\0') {
        return false;
    }
    return raw[0] == '1' || raw[0] == 'y' || raw[0] == 'Y' || raw[0] == 't' || raw[0] == 'T';
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

    std::vector<macos_keyboard_injector::ModifierMapping> modifiers;
    modifiers.reserve(chord.modifiers.size());
    for (uint32_t modifierVk : chord.modifiers) {
        macos_keyboard_injector::ModifierMapping mapping{};
        if (!macos_keyboard_injector::ResolveModifierMapping(modifierVk, &mapping)) {
            return false;
        }
        modifiers.push_back(mapping);
    }

    uint16_t keyCode = 0;
    uint64_t keyModifierFlag = 0;
    if (!macos_keyboard_injector::ResolveKeyCode(chord.key, &keyCode, &keyModifierFlag)) {
        return false;
    }

    if (IsKeyboardInjectorDryRunEnabled()) {
        return true;
    }
    if (!AXIsProcessTrusted()) {
        return false;
    }

    uint64_t currentFlags = 0;
    for (const macos_keyboard_injector::ModifierMapping& modifier : modifiers) {
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
