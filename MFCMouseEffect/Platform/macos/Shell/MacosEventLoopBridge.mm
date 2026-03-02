#include "Platform/macos/Shell/MacosEventLoopBridge.h"

#if defined(__APPLE__)
#import <AppKit/AppKit.h>
#endif

namespace mousefx::platform::macos {

void EnsureMacosApplicationReady() {
#if defined(__APPLE__)
    [NSApplication sharedApplication];
#endif
}

void RunMacosApplicationEventLoop() {
#if defined(__APPLE__)
    [NSApp run];
#endif
}

void StopMacosApplicationEventLoop() {
#if defined(__APPLE__)
    if (NSApp == nil) {
        return;
    }

    [NSApp stop:nil];

    NSEvent* wakeEvent = [NSEvent otherEventWithType:NSEventTypeApplicationDefined
                                            location:NSZeroPoint
                                       modifierFlags:0
                                           timestamp:0
                                        windowNumber:0
                                             context:nil
                                             subtype:0
                                               data1:0
                                               data2:0];
    if (wakeEvent != nil) {
        [NSApp postEvent:wakeEvent atStart:NO];
    }
#endif
}

} // namespace mousefx::platform::macos
