#include "pch.h"

#include "Platform/macos/Shell/MacosTrayService.h"

#include "MouseFx/Core/Shell/IAppShellHost.h"
#include "Platform/macos/Shell/MacosTrayMenuLocalization.h"

#if defined(__APPLE__)
#import <AppKit/AppKit.h>
#import <dispatch/dispatch.h>

#include <cstddef>
#include <cstdlib>
#include <string_view>

@interface MfxMacTrayActionBridge : NSObject {
@private
    mousefx::IAppShellHost* host_;
}

- (instancetype)initWithHost:(mousefx::IAppShellHost*)host;
- (void)onOpenSettings:(id)sender;
- (void)onExit:(id)sender;

@end

@implementation MfxMacTrayActionBridge

- (instancetype)initWithHost:(mousefx::IAppShellHost*)host {
    self = [super init];
    if (self != nil) {
        host_ = host;
    }
    return self;
}

- (void)onOpenSettings:(id)sender {
    (void)sender;
    if (host_ != nullptr) {
        host_->OpenSettingsFromShell();
    }
}

- (void)onExit:(id)sender {
    (void)sender;
    if (host_ != nullptr) {
        host_->RequestExitFromShell();
    }
}

@end

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

} // namespace
#endif

namespace mousefx {

#if defined(__APPLE__)

struct MacosTrayService::Impl {
    IAppShellHost* host = nullptr;
    NSStatusItem* statusItem = nil;
    NSMenu* menu = nil;
    MfxMacTrayActionBridge* actionBridge = nil;
    bool started = false;
};

MacosTrayService::MacosTrayService()
    : impl_(std::make_unique<Impl>()) {}

MacosTrayService::~MacosTrayService() {
    Stop();
}

bool MacosTrayService::Start(IAppShellHost* host, bool showTrayIcon) {
    if (!impl_ || host == nullptr) {
        return false;
    }
    if (impl_->started) {
        return true;
    }

    impl_->host = host;
    if (!showTrayIcon) {
        impl_->started = true;
        return true;
    }

    __block bool started = true;
    RunOnMainThreadSync(^{
      const MacosTrayMenuText menuText = ResolveMacosTrayMenuText();
      NSString* settingsTitle = NsStringFromUtf8OrDefault(menuText.settingsTitle, @"Settings");
      NSString* exitTitle = NsStringFromUtf8OrDefault(menuText.exitTitle, @"Exit");
      NSString* tooltip = NsStringFromUtf8OrDefault(menuText.tooltip, @"MFCMouseEffect");

      [NSApplication sharedApplication];
      [NSApp setActivationPolicy:NSApplicationActivationPolicyAccessory];

      impl_->statusItem = [[[NSStatusBar systemStatusBar] statusItemWithLength:NSVariableStatusItemLength] retain];
      impl_->actionBridge = [[MfxMacTrayActionBridge alloc] initWithHost:host];
      impl_->menu = [[NSMenu alloc] initWithTitle:@"MFCMouseEffect"];
      if (impl_->statusItem == nil || impl_->actionBridge == nil || impl_->menu == nil) {
          started = false;
          return;
      }

      NSMenuItem* settingsItem = [[NSMenuItem alloc] initWithTitle:settingsTitle
                                                             action:@selector(onOpenSettings:)
                                                      keyEquivalent:@","];
      [settingsItem setKeyEquivalentModifierMask:NSEventModifierFlagCommand];
      [settingsItem setTarget:impl_->actionBridge];
      [impl_->menu addItem:settingsItem];
      [settingsItem release];

      [impl_->menu addItem:[NSMenuItem separatorItem]];

      NSMenuItem* exitItem = [[NSMenuItem alloc] initWithTitle:exitTitle
                                                         action:@selector(onExit:)
                                                  keyEquivalent:@"q"];
      [exitItem setKeyEquivalentModifierMask:NSEventModifierFlagCommand];
      [exitItem setTarget:impl_->actionBridge];
      [impl_->menu addItem:exitItem];
      [exitItem release];

      [impl_->statusItem setMenu:impl_->menu];
      NSStatusBarButton* button = [impl_->statusItem button];
      if (button != nil) {
          [button setTitle:@"MFX"];
          [button setToolTip:tooltip];
      }

      ScheduleAutoTriggerSettingsAction(impl_->menu);
    });

    if (!started) {
        Stop();
        return false;
    }

    impl_->started = true;
    return true;
}

void MacosTrayService::Stop() {
    if (!impl_ || !impl_->started) {
        return;
    }

    RunOnMainThreadSync(^{
      if (impl_->statusItem != nil) {
          [[NSStatusBar systemStatusBar] removeStatusItem:impl_->statusItem];
          [impl_->statusItem release];
          impl_->statusItem = nil;
      }
      if (impl_->menu != nil) {
          [impl_->menu release];
          impl_->menu = nil;
      }
      if (impl_->actionBridge != nil) {
          [impl_->actionBridge release];
          impl_->actionBridge = nil;
      }
    });

    impl_->host = nullptr;
    impl_->started = false;
}

void MacosTrayService::RequestExit() {
    // AppShellCore controls run-loop shutdown; tray exit action routes through host callback.
}

#else

struct MacosTrayService::Impl {};

MacosTrayService::MacosTrayService()
    : impl_(std::make_unique<Impl>()) {}

MacosTrayService::~MacosTrayService() = default;

bool MacosTrayService::Start(IAppShellHost* host, bool showTrayIcon) {
    (void)showTrayIcon;
    return host != nullptr;
}

void MacosTrayService::Stop() {
}

void MacosTrayService::RequestExit() {
}

#endif

} // namespace mousefx
