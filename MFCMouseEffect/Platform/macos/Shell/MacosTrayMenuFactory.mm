#include "pch.h"

#include "Platform/macos/Shell/MacosTrayMenuFactory.h"

#include "Platform/macos/Shell/MacosTrayRuntimeHelpers.h"

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

namespace mousefx::macos_tray {

#if defined(__APPLE__)
bool BuildMacosTrayMenu(
    IAppShellHost* host,
    const MacosTrayMenuText& menuText,
    MacosTrayMenuObjects* outObjects) {
    if (host == nullptr || outObjects == nullptr) {
        return false;
    }

    NSString* settingsTitle = NsStringFromUtf8OrDefault(menuText.settingsTitle, @"Settings");
    NSString* exitTitle = NsStringFromUtf8OrDefault(menuText.exitTitle, @"Exit");
    NSString* tooltip = NsStringFromUtf8OrDefault(menuText.tooltip, @"MFCMouseEffect");

    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyAccessory];

    NSStatusItem* statusItem = [[[NSStatusBar systemStatusBar] statusItemWithLength:NSVariableStatusItemLength] retain];
    MfxMacTrayActionBridge* actionBridge = [[MfxMacTrayActionBridge alloc] initWithHost:host];
    NSMenu* menu = [[NSMenu alloc] initWithTitle:@"MFCMouseEffect"];
    if (statusItem == nil || actionBridge == nil || menu == nil) {
        if (menu != nil) {
            [menu release];
        }
        if (actionBridge != nil) {
            [actionBridge release];
        }
        if (statusItem != nil) {
            [[NSStatusBar systemStatusBar] removeStatusItem:statusItem];
            [statusItem release];
        }
        return false;
    }

    NSMenuItem* settingsItem = [[NSMenuItem alloc] initWithTitle:settingsTitle
                                                           action:@selector(onOpenSettings:)
                                                    keyEquivalent:@","];
    [settingsItem setKeyEquivalentModifierMask:NSEventModifierFlagCommand];
    [settingsItem setTarget:actionBridge];
    [menu addItem:settingsItem];
    [settingsItem release];

    [menu addItem:[NSMenuItem separatorItem]];

    NSMenuItem* exitItem = [[NSMenuItem alloc] initWithTitle:exitTitle
                                                       action:@selector(onExit:)
                                                keyEquivalent:@"q"];
    [exitItem setKeyEquivalentModifierMask:NSEventModifierFlagCommand];
    [exitItem setTarget:actionBridge];
    [menu addItem:exitItem];
    [exitItem release];

    [statusItem setMenu:menu];
    NSStatusBarButton* button = [statusItem button];
    if (button != nil) {
        [button setTitle:@"MFX"];
        [button setToolTip:tooltip];
    }

    outObjects->statusItem = statusItem;
    outObjects->menu = menu;
    outObjects->actionBridge = actionBridge;
    return true;
}

void ReleaseMacosTrayMenu(MacosTrayMenuObjects* objects) {
    if (objects == nullptr) {
        return;
    }

    if (objects->statusItem != nil) {
        [[NSStatusBar systemStatusBar] removeStatusItem:objects->statusItem];
        [objects->statusItem release];
        objects->statusItem = nil;
    }
    if (objects->menu != nil) {
        [objects->menu release];
        objects->menu = nil;
    }
    if (objects->actionBridge != nil) {
        [objects->actionBridge release];
        objects->actionBridge = nil;
    }
}
#endif

} // namespace mousefx::macos_tray
