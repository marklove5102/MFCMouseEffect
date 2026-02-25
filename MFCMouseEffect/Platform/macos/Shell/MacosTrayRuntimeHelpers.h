#pragma once

#include <string>

#if defined(__APPLE__)
#import <AppKit/AppKit.h>
#import <dispatch/dispatch.h>
#endif

namespace mousefx::macos_tray {

#if defined(__APPLE__)
void RunOnMainThreadSync(dispatch_block_t block);
NSString* NsStringFromUtf8OrDefault(const std::string& text, NSString* fallback);
void ScheduleAutoTriggerSettingsAction(NSMenu* menu);
#endif

} // namespace mousefx::macos_tray
