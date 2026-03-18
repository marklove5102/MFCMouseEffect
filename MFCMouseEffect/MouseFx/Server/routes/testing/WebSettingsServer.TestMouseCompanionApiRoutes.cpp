#include "pch.h"
#include "WebSettingsServer.TestMouseCompanionApiRoutes.h"

#include <algorithm>
#include <cstdint>
#include <string>

#include "MouseFx/Core/Control/AppController.h"
#include "MouseFx/Core/Pet/PetActionCoverageAnalyzer.h"
#include "MouseFx/Core/Pet/PetActionLibrary.h"
#include "MouseFx/Core/Pet/PetGlbSkeletonLoader.h"
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
        {"visual_host_active", status.visualHostActive},
        {"visual_model_loaded", status.visualModelLoaded},
        {"model_loaded", status.modelLoaded},
        {"action_library_loaded", status.actionLibraryLoaded},
        {"effect_profile_loaded", status.effectProfileLoaded},
        {"appearance_profile_loaded", status.appearanceProfileLoaded},
        {"pose_binding_configured", status.poseBindingConfigured},
        {"skeleton_bone_count", status.skeletonBoneCount},
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
    });
}

json BuildActionCoverageJson(const AppController::MouseCompanionRuntimeStatus& status) {
    json out = json::object();
    out["ready"] = false;
    out["error"] = "";
    out["expected_action_count"] = 0;
    out["covered_action_count"] = 0;
    out["missing_action_count"] = 0;
    out["skeleton_bone_count"] = 0;
    out["total_track_count"] = 0;
    out["mapped_track_count"] = 0;
    out["overall_coverage_ratio"] = 0.0;
    out["missing_actions"] = json::array();
    out["missing_bone_names"] = json::array();
    out["actions"] = json::array();

    if (!status.modelLoaded || status.loadedModelPath.empty()) {
        out["error"] = "runtime_model_unavailable";
        return out;
    }
    if (!status.actionLibraryLoaded || status.loadedActionLibraryPath.empty()) {
        out["error"] = "action_library_unavailable";
        return out;
    }

    pet::SkeletonDesc skeleton{};
    std::string error;
    if (!pet::LoadSkeletonFromGlb(status.loadedModelPath, &skeleton, &error)) {
        out["error"] = error.empty() ? "load_skeleton_failed" : error;
        return out;
    }

    pet::ActionLibrary library{};
    if (!pet::LoadActionLibraryFromJsonFile(status.loadedActionLibraryPath, &library, &error)) {
        out["error"] = error.empty() ? "load_action_library_failed" : error;
        return out;
    }

    const pet::ActionCoverageReport report = pet::BuildActionCoverageReport(skeleton, library);
    out["ready"] = true;
    out["error"] = "";
    out["expected_action_count"] = report.expectedActionCount;
    out["covered_action_count"] = report.coveredActionCount;
    out["missing_action_count"] = report.missingActionCount;
    out["skeleton_bone_count"] = report.skeletonBoneCount;
    out["total_track_count"] = report.totalTrackCount;
    out["mapped_track_count"] = report.totalMappedTrackCount;
    out["overall_coverage_ratio"] = report.overallCoverageRatio;
    out["missing_actions"] = report.missingActions;
    out["missing_bone_names"] = report.missingBoneNames;

    json actions = json::array();
    for (const auto& entry : report.actions) {
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
