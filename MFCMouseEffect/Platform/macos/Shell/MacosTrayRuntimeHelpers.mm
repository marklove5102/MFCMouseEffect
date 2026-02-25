#include "pch.h"

#include "Platform/macos/Shell/MacosTrayRuntimeHelpers.h"

#if defined(__APPLE__)
#include <cstddef>
#include <cstdlib>
#include <string_view>
#endif

namespace mousefx::macos_tray {

#if defined(__APPLE__)
namespace {

char ToLowerAscii(char c) {
    if (c >= 'A' && c <= 'Z') {
        return static_cast<char>(c - 'A' + 'a');
    }
    return c;
}

bool EqualsIgnoreCaseAscii(std::string_view lhs, std::string_view rhs) {
    if (lhs.size() != rhs.size()) {
        return false;
    }
    for (size_t i = 0; i < lhs.size(); ++i) {
        if (ToLowerAscii(lhs[i]) != ToLowerAscii(rhs[i])) {
            return false;
        }
    }
    return true;
}

bool IsTraySettingsAutoTriggerEnabled() {
    const char* raw = std::getenv("MFX_TEST_TRAY_AUTO_TRIGGER_SETTINGS_ACTION");
    if (raw == nullptr || raw[0] == '\0') {
        return false;
    }
    const std::string_view value(raw);
    return value == "1" ||
           EqualsIgnoreCaseAscii(value, "true") ||
           EqualsIgnoreCaseAscii(value, "yes") ||
           EqualsIgnoreCaseAscii(value, "on");
}

} // namespace

void RunOnMainThreadSync(dispatch_block_t block) {
    if (!block) {
        return;
    }
    if ([NSThread isMainThread]) {
        block();
        return;
    }
    dispatch_sync(dispatch_get_main_queue(), block);
}

NSString* NsStringFromUtf8OrDefault(const std::string& text, NSString* fallback) {
    if (text.empty()) {
        return fallback;
    }
    NSString* value = [NSString stringWithUTF8String:text.c_str()];
    return (value != nil) ? value : fallback;
}

void ScheduleAutoTriggerSettingsAction(NSMenu* menu) {
    if (!IsTraySettingsAutoTriggerEnabled() || menu == nil) {
        return;
    }

    NSMenu* retainedMenu = [menu retain];
    dispatch_after(
        dispatch_time(DISPATCH_TIME_NOW, static_cast<int64_t>(120) * NSEC_PER_MSEC),
        dispatch_get_main_queue(),
        ^{
          if (retainedMenu == nil || [retainedMenu numberOfItems] <= 0) {
              [retainedMenu release];
              return;
          }

          NSMenuItem* settingsItem = [retainedMenu itemAtIndex:0];
          if (settingsItem != nil) {
              id target = [settingsItem target];
              SEL action = [settingsItem action];
              if (target != nil && action != nullptr) {
                  [NSApp sendAction:action to:target from:settingsItem];
              }
          }
          [retainedMenu release];
        });
}
#endif

} // namespace mousefx::macos_tray
