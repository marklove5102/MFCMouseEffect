#include "pch.h"
#include "WebSettingsServer.TestMouseCompanionApiRoutes.h"

#include <algorithm>
#include <cstdint>
#include <string>

#include "MouseFx/Core/Control/AppController.h"
#include "MouseFx/Core/Protocol/InputTypes.h"
#include "MouseFx/Server/http/HttpServer.h"
#include "MouseFx/Server/routes/testing/WebSettingsServer.TestRouteCommon.h"
#include "MouseFx/Utils/StringUtils.h"

using json = nlohmann::json;

namespace mousefx {
namespace {

using websettings_test_routes::IsEnabledByEnv;
using websettings_test_routes::ParseButtonOrDefault;
using websettings_test_routes::ParseInt32OrDefault;
using websettings_test_routes::ParseObjectOrEmpty;
using websettings_test_routes::SetJsonResponse;
using websettings_test_routes::SetPlainResponse;

bool IsMouseCompanionTestApiEnabled() {
    return IsEnabledByEnv("MFX_ENABLE_MOUSE_COMPANION_TEST_API");
}

MouseButton ResolveMouseButton(uint8_t rawButton) {
    switch (rawButton) {
    case 2:
        return MouseButton::Right;
    case 3:
        return MouseButton::Middle;
    case 1:
    default:
        return MouseButton::Left;
    }
}

json BuildMouseCompanionRuntimeStatusJson(const AppController::MouseCompanionRuntimeStatus& status) {
    return json({
        {"config_enabled", status.configEnabled},
        {"runtime_present", status.runtimePresent},
        {"plugin_host_ready", status.pluginHostReady},
        {"plugin_host_phase", status.pluginHostPhase},
        {"active_plugin_id", status.activePluginId},
        {"active_plugin_version", status.activePluginVersion},
        {"engine_api_version", status.engineApiVersion},
        {"compatibility_status", status.compatibilityStatus},
        {"fallback_reason", status.fallbackReason},
        {"last_plugin_event", status.lastPluginEvent},
        {"last_plugin_event_tick_ms", status.lastPluginEventTickMs},
        {"plugin_event_count", status.pluginEventCount},
        {"visual_host_active", status.visualHostActive},
        {"visual_model_loaded", status.visualModelLoaded},
        {"model_loaded", status.modelLoaded},
        {"action_library_loaded", status.actionLibraryLoaded},
        {"effect_profile_loaded", status.effectProfileLoaded},
        {"appearance_profile_loaded", status.appearanceProfileLoaded},
        {"pose_binding_configured", status.poseBindingConfigured},
        {"skeleton_bone_count", status.skeletonBoneCount},
        {"preferred_renderer_backend", status.preferredRendererBackend},
        {"selected_renderer_backend", status.selectedRendererBackend},
        {"renderer_backend_selection_reason", status.rendererBackendSelectionReason},
        {"renderer_backend_failure_reason", status.rendererBackendFailureReason},
        {"available_renderer_backends", status.availableRendererBackends},
        {"configured_model_path", status.configuredModelPath},
        {"configured_action_library_path", status.configuredActionLibraryPath},
        {"configured_effect_profile_path", status.configuredEffectProfilePath},
        {"configured_appearance_profile_path", status.configuredAppearanceProfilePath},
        {"visual_model_path", status.visualModelPath},
        {"loaded_model_path", status.loadedModelPath},
        {"loaded_model_source_format", status.loadedModelSourceFormat},
        {"loaded_action_library_path", status.loadedActionLibraryPath},
        {"loaded_effect_profile_path", status.loadedEffectProfilePath},
        {"loaded_appearance_profile_path", status.loadedAppearanceProfilePath},
        {"model_converted_to_canonical", status.modelConvertedToCanonical},
        {"model_import_diagnostics", status.modelImportDiagnostics},
        {"visual_model_load_error", status.visualModelLoadError},
        {"model_load_error", status.modelLoadError},
        {"action_library_load_error", status.actionLibraryLoadError},
        {"effect_profile_load_error", status.effectProfileLoadError},
        {"appearance_profile_load_error", status.appearanceProfileLoadError},
        {"last_action_code", status.lastActionCode},
        {"last_action_name", status.lastActionName},
        {"last_action_intensity", status.lastActionIntensity},
        {"last_action_tick_ms", status.lastActionTickMs},
        {"click_streak", status.clickStreak},
        {"click_streak_tint_amount", status.clickStreakTintAmount},
        {"click_streak_break_ms", status.clickStreakBreakMs},
        {"click_streak_decay_per_second", status.clickStreakDecayPerSecond},
    });
}

json BuildActionCoverageJson(const AppController::MouseCompanionRuntimeStatus& status) {
    json out = json::object();
    out["ready"] = status.actionCoverageReady;
    out["error"] = status.actionCoverageError;
    out["expected_action_count"] = status.actionCoverageExpectedActionCount;
    out["covered_action_count"] = status.actionCoverageCoveredActionCount;
    out["missing_action_count"] = status.actionCoverageMissingActionCount;
    out["skeleton_bone_count"] = status.actionCoverageSkeletonBoneCount;
    out["total_track_count"] = status.actionCoverageTotalTrackCount;
    out["mapped_track_count"] = status.actionCoverageMappedTrackCount;
    out["overall_coverage_ratio"] = status.actionCoverageOverallRatio;
    out["missing_actions"] = status.actionCoverageMissingActions;
    out["missing_bone_names"] = status.actionCoverageMissingBoneNames;

    json actions = json::array();
    for (const auto& entry : status.actionCoverageActions) {
        actions.push_back({
            {"action_name", entry.actionName},
            {"clip_present", entry.clipPresent},
            {"track_count", entry.trackCount},
            {"mapped_track_count", entry.mappedTrackCount},
            {"coverage_ratio", entry.coverageRatio},
            {"missing_bone_tracks", entry.missingBoneTracks},
        });
    }
    out["actions"] = std::move(actions);
    return out;
}

} // namespace

bool HandleWebSettingsTestMouseCompanionApiRoute(
    const HttpRequest& req,
    const std::string& path,
    AppController* controller,
    HttpResponse& resp) {
    if (req.method != "POST" || path != "/api/mouse-companion/test-dispatch") {
        return false;
    }

    if (!IsMouseCompanionTestApiEnabled()) {
        SetPlainResponse(resp, 404, "not found");
        return true;
    }

    if (!controller) {
        SetJsonResponse(resp, json({
            {"ok", false},
            {"error", "no controller"},
        }).dump());
        return true;
    }

    const json payload = ParseObjectOrEmpty(req.body);
    const std::string event = ToLowerAscii(TrimAscii(payload.value("event", std::string("status"))));
    const int32_t x = ParseInt32OrDefault(payload, "x", 640);
    const int32_t y = ParseInt32OrDefault(payload, "y", 360);
    const int32_t delta = ParseInt32OrDefault(payload, "delta", 120);
    const uint32_t holdMs = static_cast<uint32_t>(std::max(0, ParseInt32OrDefault(payload, "hold_ms", 420)));
    const uint8_t rawButton = ParseButtonOrDefault(payload, "button", 1);
    const int button = std::max(0, static_cast<int>(rawButton));

    ScreenPoint pt{};
    pt.x = x;
    pt.y = y;

    if (event == "status") {
        // No-op: return current runtime snapshot only.
    } else if (event == "move") {
        controller->DispatchPetMove(pt);
    } else if (event == "scroll") {
        controller->DispatchPetScroll(pt, delta);
    } else if (event == "button_down") {
        controller->DispatchPetButtonDown(pt, button);
    } else if (event == "button_up") {
        controller->DispatchPetButtonUp(pt, button);
    } else if (event == "click") {
        ClickEvent ev{};
        ev.pt = pt;
        ev.button = ResolveMouseButton(rawButton);
        controller->DispatchPetClick(ev);
    } else if (event == "hover_start") {
        controller->DispatchPetHoverStart(pt);
    } else if (event == "hover_end") {
        controller->DispatchPetHoverEnd(pt);
    } else if (event == "hold_start") {
        controller->DispatchPetHoldStart(pt, button, holdMs);
    } else if (event == "hold_update") {
        controller->DispatchPetHoldUpdate(pt, holdMs);
    } else if (event == "hold_end") {
        controller->DispatchPetHoldEnd(pt);
    } else {
        SetJsonResponse(resp, json({
            {"ok", false},
            {"error", "unsupported_event"},
            {"event", event},
            {"supported_events", json::array({
                "status",
                "move",
                "scroll",
                "button_down",
                "button_up",
                "click",
                "hover_start",
                "hover_end",
                "hold_start",
                "hold_update",
                "hold_end"})},
        }).dump());
        return true;
    }

    const AppController::MouseCompanionRuntimeStatus status =
        controller->ReadMouseCompanionRuntimeStatus();
    SetJsonResponse(resp, json({
        {"ok", true},
        {"event", event},
        {"point", {
            {"x", pt.x},
            {"y", pt.y},
        }},
        {"delta", delta},
        {"hold_ms", holdMs},
        {"button", button},
        {"runtime", BuildMouseCompanionRuntimeStatusJson(status)},
        {"action_coverage", BuildActionCoverageJson(status)},
    }).dump());
    return true;
}

} // namespace mousefx
