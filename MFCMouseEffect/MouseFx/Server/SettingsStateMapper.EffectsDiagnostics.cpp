#include "pch.h"
#include "SettingsStateMapper.Diagnostics.h"

#include "Platform/PlatformTarget.h"
#if MFX_PLATFORM_MACOS
#include "Platform/macos/Effects/MacosClickPulseWindowRegistry.h"
#include "Platform/macos/Effects/MacosEffectRenderProfile.h"
#include "Platform/macos/Effects/MacosHoldPulseOverlayRenderer.h"
#include "Platform/macos/Effects/MacosHoverPulseOverlayRenderer.h"
#include "Platform/macos/Effects/MacosScrollPulseWindowRegistry.h"
#include "Platform/macos/Effects/MacosTrailPulseWindowRegistry.h"
#endif

using json = nlohmann::json;

namespace mousefx {

json BuildEffectsRuntimeState() {
    size_t clickActiveOverlayWindows = 0;
    size_t trailActiveOverlayWindows = 0;
    size_t scrollActiveOverlayWindows = 0;
    size_t holdActiveOverlayWindows = 0;
    size_t hoverActiveOverlayWindows = 0;
#if MFX_PLATFORM_MACOS
    clickActiveOverlayWindows = macos_click_pulse::GetActiveClickPulseWindowCount();
    trailActiveOverlayWindows = macos_trail_pulse::GetActiveTrailPulseWindowCount();
    scrollActiveOverlayWindows = macos_scroll_pulse::GetActiveScrollPulseWindowCount();
    holdActiveOverlayWindows = macos_hold_pulse::GetActiveHoldPulseWindowCount();
    hoverActiveOverlayWindows = macos_hover_pulse::GetActiveHoverPulseWindowCount();
#endif

    json out;
    out["click_active_overlay_windows"] = clickActiveOverlayWindows;
    out["trail_active_overlay_windows"] = trailActiveOverlayWindows;
    out["scroll_active_overlay_windows"] = scrollActiveOverlayWindows;
    out["hold_active_overlay_windows"] = holdActiveOverlayWindows;
    out["hover_active_overlay_windows"] = hoverActiveOverlayWindows;
    out["active_overlay_windows_total"] =
        clickActiveOverlayWindows +
        trailActiveOverlayWindows +
        scrollActiveOverlayWindows +
        holdActiveOverlayWindows +
        hoverActiveOverlayWindows;
    return out;
}

json BuildEffectsProfileState(const EffectConfig& cfg) {
    json out = json::object();
    out["active"] = {
        {"click", cfg.active.click},
        {"trail", cfg.active.trail},
        {"scroll", cfg.active.scroll},
        {"hold", cfg.active.hold},
        {"hover", cfg.active.hover},
    };

#if MFX_PLATFORM_MACOS
    const auto clickProfile = macos_effect_profile::ResolveClickRenderProfile(cfg);
    const auto trailProfile = macos_effect_profile::ResolveTrailRenderProfile(cfg, cfg.active.trail);
    const auto trailThrottle = macos_effect_profile::ResolveTrailThrottleProfile(cfg, cfg.active.trail);
    const auto scrollProfile = macos_effect_profile::ResolveScrollRenderProfile(cfg);
    const auto holdProfile = macos_effect_profile::ResolveHoldRenderProfile(cfg);
    const auto hoverProfile = macos_effect_profile::ResolveHoverRenderProfile(cfg);

    out["platform"] = "macos";
    out["config_basis"] = {
        {"ripple_duration_ms", cfg.ripple.durationMs},
        {"ripple_window_size", cfg.ripple.windowSize},
        {"text_duration_ms", cfg.textClick.durationMs},
        {"trail_profile_duration_ms", cfg.GetTrailHistoryProfile(cfg.active.trail).durationMs},
        {"trail_profile_max_points", cfg.GetTrailHistoryProfile(cfg.active.trail).maxPoints},
    };
    out["click"] = {
        {"normal_size_px", clickProfile.normalSizePx},
        {"text_size_px", clickProfile.textSizePx},
        {"normal_duration_sec", clickProfile.normalDurationSec},
        {"text_duration_sec", clickProfile.textDurationSec},
        {"close_padding_ms", clickProfile.closePaddingMs},
        {"base_opacity", clickProfile.baseOpacity},
    };
    out["trail"] = {
        {"normal_size_px", trailProfile.normalSizePx},
        {"particle_size_px", trailProfile.particleSizePx},
        {"duration_sec", trailProfile.durationSec},
        {"close_padding_ms", trailProfile.closePaddingMs},
        {"base_opacity", trailProfile.baseOpacity},
    };
    out["trail_throttle"] = {
        {"min_interval_ms", trailThrottle.minIntervalMs},
        {"min_distance_px", trailThrottle.minDistancePx},
    };
    out["scroll"] = {
        {"vertical_size_px", scrollProfile.verticalSizePx},
        {"horizontal_size_px", scrollProfile.horizontalSizePx},
        {"base_duration_sec", scrollProfile.baseDurationSec},
        {"per_strength_step_sec", scrollProfile.perStrengthStepSec},
        {"close_padding_ms", scrollProfile.closePaddingMs},
        {"base_opacity", scrollProfile.baseOpacity},
    };
    out["hold"] = {
        {"size_px", holdProfile.sizePx},
        {"progress_full_ms", holdProfile.progressFullMs},
        {"breathe_duration_sec", holdProfile.breatheDurationSec},
        {"rotate_duration_sec", holdProfile.rotateDurationSec},
        {"rotate_duration_fast_sec", holdProfile.rotateDurationFastSec},
        {"base_opacity", holdProfile.baseOpacity},
    };
    out["hover"] = {
        {"size_px", hoverProfile.sizePx},
        {"breathe_duration_sec", hoverProfile.breatheDurationSec},
        {"spin_duration_sec", hoverProfile.spinDurationSec},
        {"base_opacity", hoverProfile.baseOpacity},
    };
#else
    out["platform"] = "non_macos";
#endif

    return out;
}

} // namespace mousefx
