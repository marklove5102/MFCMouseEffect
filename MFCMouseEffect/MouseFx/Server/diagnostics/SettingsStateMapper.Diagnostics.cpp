#include "pch.h"
#include "SettingsStateMapper.Diagnostics.h"

#include <filesystem>
#include <fstream>
#include <sstream>

#include "MouseFx/Core/Config/ConfigPathResolver.h"
#include "MouseFx/Core/Control/AppController.h"

using json = nlohmann::json;

namespace mousefx {

json ReadGpuRouteStatusSnapshot() {
    const std::wstring diagDir = ResolveLocalDiagDirectory();
    if (diagDir.empty()) {
        return {};
    }

    const std::filesystem::path file = std::filesystem::path(diagDir) / L"gpu_route_status_auto.json";
    std::error_code ec;
    if (!std::filesystem::exists(file, ec) || ec) {
        return {};
    }

    std::ifstream in(file, std::ios::binary);
    if (!in.is_open()) {
        return {};
    }

    std::ostringstream ss;
    ss << in.rdbuf();
    const std::string body = ss.str();
    if (body.empty()) {
        return {};
    }

    try {
        return json::parse(body);
    } catch (...) {
        return {};
    }
}

json BuildGpuRouteNotice(
    const json& routeStatus,
    const std::string& lang,
    const std::string& activeHoldType) {
    if (!routeStatus.is_object()) {
        return {};
    }

    bool fallbackApplied = false;
    if (routeStatus.contains("fallback_applied") && routeStatus["fallback_applied"].is_boolean()) {
        fallbackApplied = routeStatus["fallback_applied"].get<bool>();
    }
    if (!fallbackApplied) {
        return {};
    }

    const std::string requestedNorm = routeStatus.value(
        "requested_normalized",
        routeStatus.value("requested", std::string{}));
    const std::string effective = routeStatus.value("effective", std::string{});
    const std::string reason = routeStatus.value("reason", std::string{});

    if (!activeHoldType.empty() && activeHoldType != effective && activeHoldType != requestedNorm) {
        return {};
    }

    json notice;
    notice["level"] = "warn";
    if (lang == "zh-CN") {
        notice["message"] = std::string("GPU 路线当前不可用，已切换到兼容回退。原因：")
            + (reason.empty() ? "unknown" : reason);
    } else {
        notice["message"] = std::string("GPU route is not available on this build/device. Switched to compatible fallback. Reason: ")
            + (reason.empty() ? "unknown" : reason);
    }
    notice["reason"] = reason;
    notice["requested"] = requestedNorm;
    notice["effective"] = effective;
    return notice;
}

json BuildInputIndicatorWasmRouteStatusState(const AppController* controller) {
    if (!controller) {
        return {};
    }

    const AppController::InputIndicatorWasmRouteStatus status =
        controller->ReadInputIndicatorWasmRouteStatus();
    if (!status.routeAttempted) {
        return {};
    }

    json out = json::object();
    out["event_kind"] = status.eventKind;
    out["render_mode"] = status.renderMode;
    out["reason"] = status.reason;
    out["event_tick_ms"] = status.eventTickMs;
    out["route_attempted"] = status.routeAttempted;
    out["anchors_resolved"] = status.anchorsResolved;
    out["host_present"] = status.hostPresent;
    out["host_enabled"] = status.hostEnabled;
    out["plugin_loaded"] = status.pluginLoaded;
    out["event_supported"] = status.eventSupported;
    out["invoke_attempted"] = status.invokeAttempted;
    out["rendered_by_wasm"] = status.renderedByWasm;
    out["wasm_fallback_enabled"] = status.wasmFallbackEnabled;
    out["native_fallback_applied"] = status.nativeFallbackApplied;
    return out;
}

json BuildInputAutomationGestureRouteStatusState(const AppController* controller) {
    if (!controller || !controller->RuntimeDiagnosticsEnabled()) {
        return {};
    }

    const InputAutomationEngine::Diagnostics diag =
        controller->InputAutomation().ReadDiagnostics();

    auto buildPreviewPoints = [](const std::vector<InputAutomationEngine::GestureRouteEvent::PreviewPoint>& points) {
        json out = json::array();
        for (const auto& point : points) {
            out.push_back({
                {"x", point.x},
                {"y", point.y},
            });
        }
        return out;
    };

    json out = json::object();
    out["automation_enabled"] = diag.automationEnabled;
    out["gesture_enabled"] = diag.gestureEnabled;
    out["buttonless_gesture_enabled"] = diag.buttonlessGestureEnabled;
    out["pointer_button_down"] = diag.pointerButtonDown;
    out["gesture_mapping_count"] = diag.gestureMappingCount;
    out["buttonless_gesture_mapping_count"] = diag.buttonlessGestureMappingCount;
    out["last_stage"] = diag.lastStage;
    out["last_reason"] = diag.lastReason;
    out["last_gesture_id"] = diag.lastGestureId;
    out["last_recognized_gesture_id"] = diag.lastRecognizedGestureId;
    out["last_matched_gesture_id"] = diag.lastMatchedGestureId;
    out["last_trigger_button"] = diag.lastTriggerButton;
    out["last_matched"] = diag.lastMatched;
    out["last_injected"] = diag.lastInjected;
    out["last_used_custom"] = diag.lastUsedCustom;
    out["last_used_preset"] = diag.lastUsedPreset;
    out["last_sample_point_count"] = diag.lastSamplePointCount;
    out["last_candidate_count"] = diag.lastCandidateCount;
    out["last_best_window_start"] = diag.lastBestWindowStart;
    out["last_best_window_end"] = diag.lastBestWindowEnd;
    out["last_runner_up_score"] = diag.lastRunnerUpScore;
    out["last_preview_path_hash"] = diag.lastPreviewPathHash;
    out["last_preview_points"] = buildPreviewPoints(diag.lastPreviewPoints);
    out["last_event_seq"] = diag.lastEventSeq;
    out["last_modifiers"] = {
        {"primary", diag.lastModifiers.primary},
        {"shift", diag.lastModifiers.shift},
        {"alt", diag.lastModifiers.alt},
    };
    json recentEvents = json::array();
    for (const InputAutomationEngine::GestureRouteEvent& event : diag.recentEvents) {
        recentEvents.push_back({
            {"seq", event.seq},
            {"timestamp_ms", event.timestampMs},
            {"stage", event.stage},
            {"reason", event.reason},
            {"gesture_id", event.gestureId},
            {"recognized_gesture_id", event.recognizedGestureId},
            {"matched_gesture_id", event.matchedGestureId},
            {"trigger_button", event.triggerButton},
            {"matched", event.matched},
            {"injected", event.injected},
            {"used_custom", event.usedCustom},
            {"used_preset", event.usedPreset},
            {"sample_point_count", event.samplePointCount},
            {"candidate_count", event.candidateCount},
            {"best_window_start", event.bestWindowStart},
            {"best_window_end", event.bestWindowEnd},
            {"runner_up_score", event.runnerUpScore},
            {"preview_path_hash", event.previewPathHash},
            {"preview_points", buildPreviewPoints(event.previewPoints)},
            {"modifiers", {
                {"primary", event.modifiers.primary},
                {"shift", event.modifiers.shift},
                {"alt", event.modifiers.alt},
            }},
        });
    }
    out["recent_events"] = std::move(recentEvents);
    return out;
}

json BuildMouseCompanionRuntimeState(const AppController* controller) {
    if (!controller) {
        return {};
    }

    const AppController::MouseCompanionRuntimeStatus status =
        controller->ReadMouseCompanionRuntimeStatus();
    json out = json::object();
    out["config_enabled"] = status.configEnabled;
    out["runtime_present"] = status.runtimePresent;
    out["visual_host_active"] = status.visualHostActive;
    out["visual_model_loaded"] = status.visualModelLoaded;
    out["model_loaded"] = status.modelLoaded;
    out["action_library_loaded"] = status.actionLibraryLoaded;
    out["appearance_profile_loaded"] = status.appearanceProfileLoaded;
    out["pose_binding_configured"] = status.poseBindingConfigured;
    out["skeleton_bone_count"] = status.skeletonBoneCount;
    out["configured_model_path"] = status.configuredModelPath;
    out["configured_action_library_path"] = status.configuredActionLibraryPath;
    out["configured_appearance_profile_path"] = status.configuredAppearanceProfilePath;
    out["visual_model_path"] = status.visualModelPath;
    out["loaded_model_path"] = status.loadedModelPath;
    out["loaded_action_library_path"] = status.loadedActionLibraryPath;
    out["loaded_appearance_profile_path"] = status.loadedAppearanceProfilePath;
    out["visual_model_load_error"] = status.visualModelLoadError;
    out["model_load_error"] = status.modelLoadError;
    out["action_library_load_error"] = status.actionLibraryLoadError;
    out["appearance_profile_load_error"] = status.appearanceProfileLoadError;
    out["last_action_code"] = status.lastActionCode;
    out["last_action_name"] = status.lastActionName;
    out["last_action_intensity"] = status.lastActionIntensity;
    out["last_action_tick_ms"] = status.lastActionTickMs;
    out["action_coverage"] = {
        {"ready", status.actionCoverageReady},
        {"expected_action_count", status.actionCoverageExpectedActionCount},
        {"covered_action_count", status.actionCoverageCoveredActionCount},
        {"missing_action_count", status.actionCoverageMissingActionCount},
        {"skeleton_bone_count", status.actionCoverageSkeletonBoneCount},
        {"total_track_count", status.actionCoverageTotalTrackCount},
        {"mapped_track_count", status.actionCoverageMappedTrackCount},
        {"overall_coverage_ratio", status.actionCoverageOverallRatio},
        {"error", status.actionCoverageError},
        {"missing_actions", status.actionCoverageMissingActions},
        {"missing_bone_names", status.actionCoverageMissingBoneNames},
    };
    json actionEntries = json::array();
    for (const auto& action : status.actionCoverageActions) {
        actionEntries.push_back({
            {"action_name", action.actionName},
            {"clip_present", action.clipPresent},
            {"track_count", action.trackCount},
            {"mapped_track_count", action.mappedTrackCount},
            {"coverage_ratio", action.coverageRatio},
            {"missing_bone_tracks", action.missingBoneTracks},
        });
    }
    out["action_coverage"]["actions"] = std::move(actionEntries);
    return out;
}

} // namespace mousefx
