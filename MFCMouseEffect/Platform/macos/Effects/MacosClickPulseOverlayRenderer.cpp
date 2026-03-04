#include "pch.h"

#include "Platform/macos/Effects/MacosClickPulseOverlayRenderer.h"
#include "Platform/macos/Effects/MacosClickPulseOverlayRendererCore.h"
#include "Platform/macos/Effects/MacosOverlayRenderSupport.h"
#include "Platform/macos/Effects/MacosClickPulseWindowRegistry.h"

#include <memory>

namespace mousefx::macos_click_pulse {

namespace {

#if defined(__APPLE__)
void CloseAllClickPulseWindowsCallback(void*) {
    CloseAllClickPulseWindowsNow();
}

struct ShowClickPulseContext final {
    ClickEffectRenderCommand command{};
    std::string themeName{};
};

void ShowClickPulseOverlayCallback(void* opaque) {
    std::unique_ptr<ShowClickPulseContext> context(
        static_cast<ShowClickPulseContext*>(opaque));
    if (!context) {
        return;
    }
    ShowClickPulseOverlayOnMain(context->command, context->themeName);
}
#endif

} // namespace

void CloseAllClickPulseWindows() {
#if !defined(__APPLE__)
    return;
#else
    macos_overlay_support::RunOnMainThreadSync(&CloseAllClickPulseWindowsCallback, nullptr);
#endif
}

void ShowClickPulseOverlay(const ClickEffectRenderCommand& command, const std::string& themeName) {
#if !defined(__APPLE__)
    (void)command;
    (void)themeName;
    return;
#else
    auto* context = new ShowClickPulseContext{
        command,
        themeName,
    };
    macos_overlay_support::RunOnMainThreadAsync(&ShowClickPulseOverlayCallback, context);
#endif
}

} // namespace mousefx::macos_click_pulse
