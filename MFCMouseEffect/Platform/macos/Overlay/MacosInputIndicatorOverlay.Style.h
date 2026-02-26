#pragma once

#if defined(__APPLE__)
#import <AppKit/AppKit.h>
#endif

#include <string>

namespace mousefx::macos_input_indicator_style {

#if defined(__APPLE__)
void ConfigurePanel(NSPanel* panel);
void ConfigureContent(NSView* content);
NSTextField* CreateLabel(int sizePx);
void ApplyPanelPresentation(NSPanel* panel, NSTextField* label, int x, int y, int sizePx, const std::string& text);
#endif

} // namespace mousefx::macos_input_indicator_style
