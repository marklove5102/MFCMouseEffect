// SettingsStateMapper.cpp -- Settings state serialization extracted from WebSettingsServer

#include "pch.h"
#include "SettingsStateMapper.h"

#include <filesystem>
#include <fstream>
#include <sstream>

#include "MouseFx/Core/Control/AppController.h"
#include "MouseFx/Core/Config/ConfigPathResolver.h"
#include "MouseFx/Core/Wasm/WasmCommandRenderer.h"
#include "MouseFx/Core/Wasm/WasmEffectHost.h"
#include "MouseFx/Core/Json/JsonFacade.h"
#include "MouseFx/Utils/StringUtils.h"
#include "Platform/PlatformTarget.h"

using json = nlohmann::json;

namespace mousefx {

static bool IsWasmRenderSupportedOnCurrentPlatform() {
    static const bool supported = [] {
        auto renderer = wasm::CreatePlatformWasmCommandRenderer();
        return renderer && renderer->SupportsRendering();
    }();
    return supported;
}

static json ReadGpuRouteStatusSnapshot() {
    const std::wstring diagDir = ResolveLocalDiagDirectory();
    if (diagDir.empty()) return {};

    const std::filesystem::path file = std::filesystem::path(diagDir) / L"gpu_route_status_auto.json";
    std::error_code ec;
    if (!std::filesystem::exists(file, ec) || ec) {
        return {};
    }

    std::ifstream in(file, std::ios::binary);
    if (!in.is_open()) return {};

    std::ostringstream ss;
    ss << in.rdbuf();
    const std::string body = ss.str();
    if (body.empty()) return {};

    try {
        return json::parse(body);
    } catch (...) {
        return {};
    }
}

static json BuildGpuRouteNotice(
    const json& routeStatus,
    const std::string& lang,
    const std::string& activeHoldType) {
    if (!routeStatus.is_object()) return {};

    bool fallbackApplied = false;
    if (routeStatus.contains("fallback_applied") && routeStatus["fallback_applied"].is_boolean()) {
        fallbackApplied = routeStatus["fallback_applied"].get<bool>();
    }
    if (!fallbackApplied) return {};

    const std::string requestedNorm = routeStatus.value(
        "requested_normalized",
        routeStatus.value("requested", std::string{}));
    const std::string effective = routeStatus.value("effective", std::string{});
    const std::string reason = routeStatus.value("reason", std::string{});

    // Show notice only when it matches current active hold route.
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

static json BuildWasmState(const EffectConfig& cfg, const AppController* controller) {
    if (!controller) {
        return {};
    }
    const wasm::WasmEffectHost* host = controller->WasmHost();
    if (!host) {
        return {};
    }

    const wasm::HostDiagnostics& diag = host->Diagnostics();
    const wasm::ExecutionBudget runtimeBudget = host->GetExecutionBudget();
    json out;
    out["enabled"] = diag.enabled;
    out["runtime_backend"] = diag.runtimeBackend;
    out["runtime_fallback_reason"] = diag.runtimeFallbackReason;
    out["plugin_loaded"] = diag.pluginLoaded;
    out["plugin_api_version"] = diag.pluginApiVersion;
    out["active_plugin_id"] = diag.activePluginId;
    out["active_plugin_name"] = diag.activePluginName;
    out["configured_enabled"] = cfg.wasm.enabled;
    out["fallback_to_builtin_click"] = cfg.wasm.fallbackToBuiltinClick;
    out["configured_manifest_path"] = cfg.wasm.manifestPath;
    out["configured_catalog_root_path"] = cfg.wasm.catalogRootPath;
    out["configured_output_buffer_bytes"] = cfg.wasm.outputBufferBytes;
    out["configured_max_commands"] = cfg.wasm.maxCommands;
    out["configured_max_execution_ms"] = cfg.wasm.maxEventExecutionMs;
    out["invoke_supported"] = true;
    out["render_supported"] = IsWasmRenderSupportedOnCurrentPlatform();
    out["runtime_output_buffer_bytes"] = runtimeBudget.outputBufferBytes;
    out["runtime_max_commands"] = runtimeBudget.maxCommands;
    out["runtime_max_execution_ms"] = runtimeBudget.maxEventExecutionMs;
    out["active_manifest_path"] = Utf16ToUtf8(diag.activeManifestPath.c_str());
    out["active_wasm_path"] = Utf16ToUtf8(diag.activeWasmPath.c_str());
    out["last_call_duration_us"] = diag.lastCallDurationMicros;
    out["last_output_bytes"] = diag.lastOutputBytes;
    out["last_command_count"] = diag.lastCommandCount;
    out["last_call_exceeded_budget"] = diag.lastCallExceededBudget;
    out["last_call_rejected_by_budget"] = diag.lastCallRejectedByBudget;
    out["last_output_truncated_by_budget"] = diag.lastOutputTruncatedByBudget;
    out["last_command_truncated_by_budget"] = diag.lastCommandTruncatedByBudget;
    out["last_budget_reason"] = diag.lastBudgetReason;
    out["last_parse_error"] = wasm::CommandParseErrorToString(diag.lastParseError);
    out["last_rendered_by_wasm"] = diag.lastRenderedByWasm;
    out["last_executed_text_commands"] = diag.lastExecutedTextCommands;
    out["last_executed_image_commands"] = diag.lastExecutedImageCommands;
    out["last_throttled_render_commands"] = diag.lastThrottledRenderCommands;
    out["last_throttled_by_capacity_render_commands"] = diag.lastThrottledByCapacityRenderCommands;
    out["last_throttled_by_interval_render_commands"] = diag.lastThrottledByIntervalRenderCommands;
    out["last_dropped_render_commands"] = diag.lastDroppedRenderCommands;
    out["last_render_error"] = diag.lastRenderError;
    out["last_error"] = diag.lastError;
    return out;
}

const char* InputCaptureReasonToCode(AppController::InputCaptureFailureReason reason) {
    using Reason = AppController::InputCaptureFailureReason;
    switch (reason) {
    case Reason::PermissionDenied:
        return "permission_denied";
    case Reason::Unsupported:
        return "unsupported";
    case Reason::StartFailed:
        return "start_failed";
    case Reason::None:
    default:
        return "none";
    }
}

json BuildInputCaptureState(const AppController* controller, const std::string& lang) {
    if (!controller) {
        return {};
    }

    const AppController::InputCaptureRuntimeStatus status = controller->InputCaptureStatus();
    json out;
    out["active"] = status.active;
    out["error"] = status.error;
    out["reason"] = InputCaptureReasonToCode(status.reason);
    out["degraded"] = !status.active;
#if MFX_PLATFORM_MACOS
    out["required_permissions"] = json::array({"accessibility", "input_monitoring"});
#endif

    if (status.active || status.reason == AppController::InputCaptureFailureReason::None) {
        return out;
    }

    json notice;
    notice["level"] = "warn";
    notice["reason"] = InputCaptureReasonToCode(status.reason);
    notice["error"] = status.error;

    const bool zh = (lang == "zh-CN");
    switch (status.reason) {
    case AppController::InputCaptureFailureReason::PermissionDenied:
        if (zh) {
            notice["message"] =
                "全局输入采集已降级：未授予 macOS 权限。请在“系统设置 > 隐私与安全性”中为 "
                "MFCMouseEffect 同时开启“辅助功能”和“输入监控”，然后重启应用。";
        } else {
            notice["message"] =
                "Global input capture is degraded: macOS permissions are missing. "
                "Grant both Accessibility and Input Monitoring to MFCMouseEffect in "
                "System Settings > Privacy & Security, then restart the app.";
        }
        break;
    case AppController::InputCaptureFailureReason::Unsupported:
        if (zh) {
            notice["message"] =
                "当前平台暂不支持全局输入采集，已自动降级为仅保留可用功能。";
        } else {
            notice["message"] =
                "Global input capture is not supported on this platform. "
                "The app is running in degraded mode with available features only.";
        }
        break;
    case AppController::InputCaptureFailureReason::StartFailed:
    default:
        if (zh) {
            notice["message"] =
                std::string("全局输入采集启动失败，当前以降级模式运行。错误码：") +
                std::to_string(status.error) + "。";
        } else {
            notice["message"] =
                std::string("Global input capture failed to start. Running in degraded mode. Error code: ") +
                std::to_string(status.error) + ".";
        }
        break;
    }

    out["notice"] = notice;
    return out;
}

std::string BuildSettingsStateJson(const EffectConfig& cfg, const AppController* controller) {
    const std::string lang = EnsureUtf8(cfg.uiLanguage);

    json out;
    out["ui_language"] = lang;
    out["theme"] = EnsureUtf8(cfg.theme);
    out["hold_follow_mode"] = EnsureUtf8(cfg.holdFollowMode);
    out["hold_presenter_backend"] = EnsureUtf8(cfg.holdPresenterBackend);
    const std::string activeHoldType = EnsureUtf8(cfg.active.hold);
    out["active"] = {
        {"click", EnsureUtf8(cfg.active.click)},
        {"trail", EnsureUtf8(cfg.active.trail)},
        {"scroll", EnsureUtf8(cfg.active.scroll)},
        {"hold", activeHoldType},
        {"hover", EnsureUtf8(cfg.active.hover)},
    };

    // Text content: flatten to comma-separated UTF-8.
    std::string text;
    for (size_t i = 0; i < cfg.textClick.texts.size(); ++i) {
        std::string utf8 = Utf16ToUtf8(cfg.textClick.texts[i].c_str());
        if (i > 0) text += ",";
        text += utf8;
    }
    out["text_content"] = text;
    out["text_font_size"] = cfg.textClick.fontSize;

    out["trail_style"] = EnsureUtf8(cfg.trailStyle);
    out["trail_profiles"] = {
        {"line", {{"duration_ms", cfg.trailProfiles.line.durationMs}, {"max_points", cfg.trailProfiles.line.maxPoints}}},
        {"streamer", {{"duration_ms", cfg.trailProfiles.streamer.durationMs}, {"max_points", cfg.trailProfiles.streamer.maxPoints}}},
        {"electric", {{"duration_ms", cfg.trailProfiles.electric.durationMs}, {"max_points", cfg.trailProfiles.electric.maxPoints}}},
        {"meteor", {{"duration_ms", cfg.trailProfiles.meteor.durationMs}, {"max_points", cfg.trailProfiles.meteor.maxPoints}}},
        {"tubes", {{"duration_ms", cfg.trailProfiles.tubes.durationMs}, {"max_points", cfg.trailProfiles.tubes.maxPoints}}},
    };

    out["trail_params"] = {
        {"streamer", {{"glow_width_scale", cfg.trailParams.streamer.glowWidthScale}, {"core_width_scale", cfg.trailParams.streamer.coreWidthScale}, {"head_power", cfg.trailParams.streamer.headPower}}},
        {"electric", {{"amplitude_scale", cfg.trailParams.electric.amplitudeScale}, {"fork_chance", cfg.trailParams.electric.forkChance}}},
        {"meteor", {{"spark_rate_scale", cfg.trailParams.meteor.sparkRateScale}, {"spark_speed_scale", cfg.trailParams.meteor.sparkSpeedScale}}},
        {"idle_fade_start_ms", cfg.trailParams.idleFade.startMs},
        {"idle_fade_end_ms", cfg.trailParams.idleFade.endMs},
    };
    out["input_indicator"] = {
        {"enabled", cfg.inputIndicator.enabled},
        {"keyboard_enabled", cfg.inputIndicator.keyboardEnabled},
        {"position_mode", EnsureUtf8(cfg.inputIndicator.positionMode)},
        {"offset_x", cfg.inputIndicator.offsetX},
        {"offset_y", cfg.inputIndicator.offsetY},
        {"absolute_x", cfg.inputIndicator.absoluteX},
        {"absolute_y", cfg.inputIndicator.absoluteY},
        {"target_monitor", EnsureUtf8(cfg.inputIndicator.targetMonitor)},
        {"key_display_mode", cfg.inputIndicator.keyDisplayMode},
        {"key_label_layout_mode", cfg.inputIndicator.keyLabelLayoutMode},
        // Per-monitor overrides
        {"per_monitor_overrides", [&](){
            json j = json::object();
            for(auto& [k, v] : cfg.inputIndicator.perMonitorOverrides) j[k] = {{"enabled", v.enabled}, {"absolute_x", v.absoluteX}, {"absolute_y", v.absoluteY}};
            return j;
        }()},
        {"size_px", cfg.inputIndicator.sizePx},
        {"duration_ms", cfg.inputIndicator.durationMs}
    };
    out["automation"] = {
        {"enabled", cfg.automation.enabled},
        {"mouse_mappings", [&](){
            json arr = json::array();
            for (const auto& binding : cfg.automation.mouseMappings) {
                json scopes = json::array();
                for (const auto& scope : binding.appScopes) {
                    scopes.push_back(EnsureUtf8(scope));
                }
                const std::string legacyScope = scopes.empty()
                    ? std::string("all")
                    : scopes.front().get<std::string>();
                arr.push_back({
                    {"enabled", binding.enabled},
                    {"trigger", EnsureUtf8(binding.trigger)},
                    {"app_scope", legacyScope},
                    {"app_scopes", scopes},
                    {"keys", EnsureUtf8(binding.keys)},
                });
            }
            return arr;
        }()},
        {"gesture", {
            {"enabled", cfg.automation.gesture.enabled},
            {"trigger_button", EnsureUtf8(cfg.automation.gesture.triggerButton)},
            {"min_stroke_distance_px", cfg.automation.gesture.minStrokeDistancePx},
            {"sample_step_px", cfg.automation.gesture.sampleStepPx},
            {"max_directions", cfg.automation.gesture.maxDirections},
            {"mappings", [&](){
                json arr = json::array();
                for (const auto& binding : cfg.automation.gesture.mappings) {
                    json scopes = json::array();
                    for (const auto& scope : binding.appScopes) {
                        scopes.push_back(EnsureUtf8(scope));
                    }
                    const std::string legacyScope = scopes.empty()
                        ? std::string("all")
                        : scopes.front().get<std::string>();
                    arr.push_back({
                        {"enabled", binding.enabled},
                        {"trigger", EnsureUtf8(binding.trigger)},
                        {"app_scope", legacyScope},
                        {"app_scopes", scopes},
                        {"keys", EnsureUtf8(binding.keys)},
                    });
                }
                return arr;
            }()},
        }},
    };

    const json routeStatus = ReadGpuRouteStatusSnapshot();
    if (routeStatus.is_object()) {
        out["gpu_route_status"] = routeStatus;
    }
    const json routeNotice = BuildGpuRouteNotice(routeStatus, lang.empty() ? "zh-CN" : lang, activeHoldType);
    if (routeNotice.is_object() && !routeNotice.empty()) {
        out["gpu_route_notice"] = routeNotice;
    }

    const json wasmState = BuildWasmState(cfg, controller);
    if (wasmState.is_object() && !wasmState.empty()) {
        out["wasm"] = wasmState;
    }

    const json inputCaptureState = BuildInputCaptureState(controller, lang.empty() ? "zh-CN" : lang);
    if (inputCaptureState.is_object() && !inputCaptureState.empty()) {
        out["input_capture"] = inputCaptureState;
        if (inputCaptureState.contains("notice")) {
            out["input_capture_notice"] = inputCaptureState["notice"];
        }
    }

    return out.dump();
}

std::string ApplySettingsStateJson(AppController* controller, const std::string& body) {
    if (!controller) return json({{"ok", false}, {"error", "no controller"}}).dump();

    json j;
    try {
        j = json::parse(body);
    } catch (...) {
        return json({{"ok", false}, {"error", "invalid json"}}).dump();
    }

    json cmd;
    cmd["cmd"] = "apply_settings";
    cmd["payload"] = j;
    controller->HandleCommand(cmd.dump());
    return json({{"ok", true}}).dump();
}

} // namespace mousefx
