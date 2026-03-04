#include "pch.h"

#include "Platform/macos/Effects/MacosTrailPulseOverlayRenderer.h"
#include "Platform/macos/Effects/MacosTrailPulseOverlayRendererCore.h"
#include "Platform/macos/Effects/MacosOverlayRenderSupport.h"
#include "Platform/macos/Effects/MacosTrailPulseWindowRegistry.h"

#include <memory>

namespace mousefx::macos_trail_pulse {

namespace {

#if defined(__APPLE__)
void CloseAllTrailPulseWindowsCallback(void*) {
    CloseAllTrailPulseWindowsNow();
}

struct ShowTrailPulseContext final {
    TrailEffectRenderCommand command{};
    std::string themeName{};
};

void ShowTrailPulseOverlayCallback(void* opaque) {
    std::unique_ptr<ShowTrailPulseContext> context(
        static_cast<ShowTrailPulseContext*>(opaque));
    if (!context) {
        return;
    }
    ShowTrailPulseOverlayOnMain(context->command, context->themeName);
}
#endif

} // namespace

void CloseAllTrailPulseWindows() {
#if !defined(__APPLE__)
    return;
#else
    macos_overlay_support::RunOnMainThreadSync(&CloseAllTrailPulseWindowsCallback, nullptr);
#endif
}

void ShowTrailPulseOverlay(const TrailEffectRenderCommand& command, const std::string& themeName) {
#if !defined(__APPLE__)
    (void)command;
    (void)themeName;
    return;
#else
    auto* context = new ShowTrailPulseContext{
        command,
        themeName,
    };
    macos_overlay_support::RunOnMainThreadAsync(&ShowTrailPulseOverlayCallback, context);
#endif
}

} // namespace mousefx::macos_trail_pulse
