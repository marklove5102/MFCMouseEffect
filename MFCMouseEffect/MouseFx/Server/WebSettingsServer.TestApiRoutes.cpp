#include "pch.h"
#include "WebSettingsServer.TestApiRoutes.h"

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <string>
#include <vector>

#include "MouseFx/Core/Automation/AutomationActionIdNormalizer.h"
#include "MouseFx/Core/Automation/AppScopeUtils.h"
#include "MouseFx/Core/Automation/BindingMatchUtils.h"
#include "MouseFx/Core/Automation/ShortcutTextFormatter.h"
#include "MouseFx/Core/Control/AppController.h"
#include "MouseFx/Core/Json/JsonFacade.h"
#include "MouseFx/Core/Protocol/InputTypes.h"
#include "MouseFx/Core/Wasm/WasmEffectHost.h"
#include "MouseFx/Core/Wasm/WasmEventInvokeExecutor.h"
#include "MouseFx/Server/HttpServer.h"
#include "MouseFx/Utils/StringUtils.h"
#include "Platform/macos/System/MacosVirtualKeyMapper.h"

using json = nlohmann::json;

namespace mousefx {
namespace {

void SetJsonResponse(HttpResponse& resp, const std::string& body) {
    resp.statusCode = 200;
    resp.contentType = "application/json; charset=utf-8";
    resp.body = body;
}

void SetPlainResponse(HttpResponse& resp, int code, const std::string& body) {
    resp.statusCode = code;
    resp.contentType = "text/plain; charset=utf-8";
    resp.body = body;
}

json ParseObjectOrEmpty(const std::string& body) {
    if (body.empty()) {
        return json::object();
    }
    try {
        json parsed = json::parse(body);
        if (parsed.is_object()) {
            return parsed;
        }
    } catch (...) {
    }
    return json::object();
}

char ToLowerAscii(char c) {
    if (c >= 'A' && c <= 'Z') {
        return static_cast<char>(c - 'A' + 'a');
    }
    return c;
}

bool EqualsIgnoreCaseAscii(std::string_view lhs, std::string_view rhs) {
    if (lhs.size() != rhs.size()) {
        return false;
    }
    for (size_t i = 0; i < lhs.size(); ++i) {
        if (ToLowerAscii(lhs[i]) != ToLowerAscii(rhs[i])) {
            return false;
        }
    }
    return true;
}

bool IsEnabledByEnv(const char* name) {
    const char* raw = std::getenv(name);
    if (raw == nullptr || raw[0] == '\0') {
        return false;
    }

    const std::string_view value(raw);
    return value == "1" ||
           EqualsIgnoreCaseAscii(value, "true") ||
           EqualsIgnoreCaseAscii(value, "yes") ||
           EqualsIgnoreCaseAscii(value, "on");
}

bool IsAutomationScopeTestApiEnabled() {
    return IsEnabledByEnv("MFX_ENABLE_AUTOMATION_SCOPE_TEST_API");
}

bool IsAutomationShortcutTestApiEnabled() {
    return IsEnabledByEnv("MFX_ENABLE_AUTOMATION_SHORTCUT_TEST_API");
}

bool IsAutomationInjectionTestApiEnabled() {
    return IsEnabledByEnv("MFX_ENABLE_AUTOMATION_INJECTION_TEST_API");
}

bool IsInputIndicatorTestApiEnabled() {
    return IsEnabledByEnv("MFX_ENABLE_INPUT_INDICATOR_TEST_API");
}

bool IsWasmTestDispatchApiEnabled() {
    return IsEnabledByEnv("MFX_ENABLE_WASM_TEST_DISPATCH_API");
}

bool ParseBooleanOrDefault(const json& payload, const char* key, bool defaultValue) {
    if (!payload.contains(key)) {
        return defaultValue;
    }
    if (payload[key].is_boolean()) {
        return payload[key].get<bool>();
    }
    if (payload[key].is_number_integer()) {
        return payload[key].get<int>() != 0;
    }
    return defaultValue;
}

int32_t ParseInt32OrDefault(const json& payload, const char* key, int32_t defaultValue) {
    if (!payload.contains(key) || !payload[key].is_number_integer()) {
        return defaultValue;
    }
    const int64_t raw = payload[key].get<int64_t>();
    if (raw < static_cast<int64_t>(std::numeric_limits<int32_t>::min())) {
        return std::numeric_limits<int32_t>::min();
    }
    if (raw > static_cast<int64_t>(std::numeric_limits<int32_t>::max())) {
        return std::numeric_limits<int32_t>::max();
    }
    return static_cast<int32_t>(raw);
}

uint8_t ParseButtonOrDefault(const json& payload, const char* key, uint8_t defaultValue) {
    if (!payload.contains(key) || !payload[key].is_number_integer()) {
        return defaultValue;
    }
    const int64_t raw = payload[key].get<int64_t>();
    if (raw <= 0) {
        return 0;
    }
    if (raw > static_cast<int64_t>(std::numeric_limits<uint8_t>::max())) {
        return std::numeric_limits<uint8_t>::max();
    }
    return static_cast<uint8_t>(raw);
}

std::vector<std::string> ParseAppScopes(const json& payload) {
    std::vector<std::string> out;
    if (payload.contains("app_scopes") && payload["app_scopes"].is_array()) {
        for (const auto& item : payload["app_scopes"]) {
            if (item.is_string()) {
                out.push_back(item.get<std::string>());
            }
        }
    }
    if (out.empty() && payload.contains("app_scope") && payload["app_scope"].is_string()) {
        out.push_back(payload["app_scope"].get<std::string>());
    }
    return out;
}

std::string ParseProcessBaseName(const json& payload) {
    if (!payload.contains("process") || !payload["process"].is_string()) {
        return {};
    }
    return payload["process"].get<std::string>();
}

std::string ParseShortcutKeys(const json& payload) {
    if (!payload.contains("keys") || !payload["keys"].is_string()) {
        return {};
    }
    return TrimAscii(payload["keys"].get<std::string>());
}

std::string ParseStringOrDefault(const json& payload, const char* key, std::string defaultValue = {}) {
    if (!payload.contains(key) || !payload[key].is_string()) {
        return defaultValue;
    }
    return payload[key].get<std::string>();
}

std::vector<std::string> ParseActionHistory(const json& payload) {
    std::vector<std::string> actions;
    if (payload.contains("history") && payload["history"].is_array()) {
        for (const auto& item : payload["history"]) {
            if (item.is_string()) {
                actions.push_back(item.get<std::string>());
            }
        }
    }
    if (actions.empty() && payload.contains("action") && payload["action"].is_string()) {
        actions.push_back(payload["action"].get<std::string>());
    }
    return actions;
}

std::vector<AutomationKeyBinding> ParseAutomationMappings(const json& payload) {
    std::vector<AutomationKeyBinding> mappings;
    if (!payload.contains("mappings") || !payload["mappings"].is_array()) {
        return mappings;
    }

    for (const auto& item : payload["mappings"]) {
        if (!item.is_object()) {
            continue;
        }

        AutomationKeyBinding binding{};
        binding.enabled = ParseBooleanOrDefault(item, "enabled", true);
        binding.trigger = TrimAscii(ParseStringOrDefault(item, "trigger"));
        binding.keys = TrimAscii(ParseStringOrDefault(item, "keys"));
        binding.appScopes = ParseAppScopes(item);
        mappings.push_back(std::move(binding));
    }

    return mappings;
}

std::vector<automation_match::ActionHistoryEntry> NormalizeMouseHistoryEntries(
    const std::vector<std::string>& actions,
    std::vector<std::string>* normalizedOut) {
    std::vector<automation_match::ActionHistoryEntry> history;
    if (normalizedOut) {
        normalizedOut->clear();
    }

    const auto baseTime = std::chrono::steady_clock::now();
    for (size_t i = 0; i < actions.size(); ++i) {
        std::string normalized = automation_ids::NormalizeMouseActionId(actions[i]);
        if (normalized.empty()) {
            continue;
        }
        if (normalizedOut) {
            normalizedOut->push_back(normalized);
        }
        history.push_back(automation_match::ActionHistoryEntry{
            std::move(normalized),
            baseTime + std::chrono::milliseconds(static_cast<int64_t>(i * 10)),
        });
    }

    return history;
}

json BuildSelectedBindingJson(const automation_match::BindingMatchResult& match) {
    if (match.binding == nullptr) {
        return nullptr;
    }

    json normalizedScopes = json::array();
    for (const auto& scope : match.binding->appScopes) {
        normalizedScopes.push_back(automation_scope::NormalizeScopeToken(scope));
    }

    return json({
        {"index", static_cast<uint64_t>(match.bindingIndex)},
        {"trigger", match.binding->trigger},
        {"keys", match.binding->keys},
        {"app_scopes", match.binding->appScopes},
        {"app_scopes_normalized", normalizedScopes},
        {"chain_length", static_cast<uint64_t>(match.chainLength)},
        {"scope_specificity", match.scopeSpecificity},
    });
}

} // namespace

bool HandleWebSettingsTestApiRoute(
    const HttpRequest& req,
    const std::string& path,
    AppController* controller,
    HttpResponse& resp) {
    if (req.method == "POST" && path == "/api/automation/test-app-scope-match") {
        if (!IsAutomationScopeTestApiEnabled()) {
            SetPlainResponse(resp, 404, "not found");
            return true;
        }

        const json payload = ParseObjectOrEmpty(req.body);
        const std::vector<std::string> rawScopes = ParseAppScopes(payload);
        const std::string processBaseName = ParseProcessBaseName(payload);
        const std::string normalizedProcess = automation_scope::NormalizeProcessName(processBaseName);

        json normalizedScopes = json::array();
        for (const std::string& scope : rawScopes) {
            normalizedScopes.push_back(automation_scope::NormalizeScopeToken(scope));
        }

        SetJsonResponse(resp, json({
            {"ok", true},
            {"process", processBaseName},
            {"process_normalized", normalizedProcess},
            {"app_scopes", rawScopes},
            {"app_scopes_normalized", normalizedScopes},
            {"matched", automation_scope::AppScopeMatchesProcess(rawScopes, processBaseName)},
            {"specificity", automation_scope::AppScopeSpecificity(rawScopes)},
        }).dump());
        return true;
    }

    if (req.method == "POST" && path == "/api/automation/test-binding-priority") {
        if (!IsAutomationScopeTestApiEnabled()) {
            SetPlainResponse(resp, 404, "not found");
            return true;
        }

        const json payload = ParseObjectOrEmpty(req.body);
        const std::vector<std::string> rawHistory = ParseActionHistory(payload);
        std::vector<std::string> normalizedHistory;
        const std::vector<automation_match::ActionHistoryEntry> history =
            NormalizeMouseHistoryEntries(rawHistory, &normalizedHistory);
        const std::vector<AutomationKeyBinding> mappings = ParseAutomationMappings(payload);
        const std::string processBaseName = ParseProcessBaseName(payload);
        const std::string normalizedProcess = automation_scope::NormalizeProcessName(processBaseName);
        const automation_match::BindingMatchResult match = automation_match::FindBestEnabledBinding(
            mappings,
            history,
            processBaseName,
            automation_match::ChainTimingLimit{},
            automation_ids::NormalizeMouseActionId);

        const json selected = BuildSelectedBindingJson(match);

        SetJsonResponse(resp, json({
            {"ok", true},
            {"process", processBaseName},
            {"process_normalized", normalizedProcess},
            {"history", rawHistory},
            {"history_normalized", normalizedHistory},
            {"mapping_count", static_cast<uint64_t>(mappings.size())},
            {"matched", match.binding != nullptr},
            {"selected_binding_index", match.binding != nullptr ? static_cast<int64_t>(match.bindingIndex) : -1},
            {"selected_chain_length", static_cast<uint64_t>(match.chainLength)},
            {"selected_scope_specificity", match.scopeSpecificity},
            {"selected_keys", match.binding != nullptr ? match.binding->keys : std::string{}},
            {"selected", selected},
        }).dump());
        return true;
    }

    if (req.method == "POST" && path == "/api/automation/test-match-and-inject") {
        if (!IsAutomationInjectionTestApiEnabled() || !IsAutomationScopeTestApiEnabled()) {
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
        const std::vector<std::string> rawHistory = ParseActionHistory(payload);
        std::vector<std::string> normalizedHistory;
        const std::vector<automation_match::ActionHistoryEntry> history =
            NormalizeMouseHistoryEntries(rawHistory, &normalizedHistory);
        const std::vector<AutomationKeyBinding> mappings = ParseAutomationMappings(payload);

        std::string processBaseName = ParseProcessBaseName(payload);
        if (processBaseName.empty()) {
            processBaseName = controller->CurrentForegroundProcessBaseName();
        }
        const std::string normalizedProcess = automation_scope::NormalizeProcessName(processBaseName);

        const automation_match::BindingMatchResult match = automation_match::FindBestEnabledBinding(
            mappings,
            history,
            processBaseName,
            automation_match::ChainTimingLimit{},
            automation_ids::NormalizeMouseActionId);

        bool injected = false;
        if (match.binding != nullptr) {
            const std::string keys = TrimAscii(match.binding->keys);
            if (!keys.empty()) {
                injected = controller->InjectShortcutForTest(keys);
            }
        }

        const json selected = BuildSelectedBindingJson(match);
        SetJsonResponse(resp, json({
            {"ok", true},
            {"process", processBaseName},
            {"process_normalized", normalizedProcess},
            {"history", rawHistory},
            {"history_normalized", normalizedHistory},
            {"mapping_count", static_cast<uint64_t>(mappings.size())},
            {"matched", match.binding != nullptr},
            {"injected", injected},
            {"selected_binding_index", match.binding != nullptr ? static_cast<int64_t>(match.bindingIndex) : -1},
            {"selected_chain_length", static_cast<uint64_t>(match.chainLength)},
            {"selected_scope_specificity", match.scopeSpecificity},
            {"selected_keys", match.binding != nullptr ? match.binding->keys : std::string{}},
            {"selected", selected},
        }).dump());
        return true;
    }

    if (req.method == "POST" && path == "/api/automation/test-shortcut-from-mac-keycode") {
        if (!IsAutomationShortcutTestApiEnabled()) {
            SetPlainResponse(resp, 404, "not found");
            return true;
        }

        const json payload = ParseObjectOrEmpty(req.body);
        const int32_t rawMacKeyCode = ParseInt32OrDefault(payload, "mac_key_code", -1);
        const bool validMacKeyCode = rawMacKeyCode >= 0 && rawMacKeyCode <= static_cast<int32_t>(std::numeric_limits<uint16_t>::max());
        const uint32_t vkCode = validMacKeyCode
            ? macos_keymap::VirtualKeyFromMacKeyCode(static_cast<uint16_t>(rawMacKeyCode))
            : 0;

        KeyEvent ev{};
        ev.vkCode = vkCode;
        ev.ctrl = ParseBooleanOrDefault(payload, "ctrl", false);
        ev.shift = ParseBooleanOrDefault(payload, "shift", false);
        ev.alt = ParseBooleanOrDefault(payload, "alt", false);
        ev.win = ParseBooleanOrDefault(
            payload,
            "cmd",
            ParseBooleanOrDefault(payload, "win", false));
        ev.meta = ev.win;
        ev.systemKey = ev.alt || ev.meta;

        const std::string shortcut = shortcut_text::FormatShortcutText(ev);
        SetJsonResponse(resp, json({
            {"ok", true},
            {"mac_key_code", rawMacKeyCode},
            {"vk_code", ev.vkCode},
            {"supported", validMacKeyCode && ev.vkCode != 0},
            {"shortcut", shortcut},
        }).dump());
        return true;
    }

    if (req.method == "POST" && path == "/api/automation/test-inject-shortcut") {
        if (!IsAutomationInjectionTestApiEnabled()) {
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
        const std::string keys = ParseShortcutKeys(payload);
        const bool accepted = !keys.empty() && controller->InjectShortcutForTest(keys);
        SetJsonResponse(resp, json({
            {"ok", true},
            {"keys", keys},
            {"accepted", accepted},
            {"process", controller->CurrentForegroundProcessBaseName()},
        }).dump());
        return true;
    }

    if (req.method == "POST" && path == "/api/input-indicator/test-mouse-labels") {
        if (!IsInputIndicatorTestApiEnabled()) {
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

        std::vector<std::string> labels;
        const bool supported = controller->IndicatorOverlay().RunMouseLabelProbe(&labels);
        const bool matched = supported &&
                             labels.size() == 3 &&
                             labels[0] == "L" &&
                             labels[1] == "R" &&
                             labels[2] == "M";
        InputIndicatorDebugState debugState{};
        const bool debugStateOk = controller->IndicatorOverlay().ReadDebugState(&debugState);

        SetJsonResponse(resp, json({
            {"ok", true},
            {"supported", supported},
            {"matched", matched},
            {"expected", json::array({"L", "R", "M"})},
            {"labels", labels},
            {"debug_state_available", debugStateOk},
            {"last_applied_label", debugStateOk ? debugState.lastAppliedLabel : std::string{}},
            {"apply_count", debugStateOk ? debugState.applyCount : 0},
        }).dump());
        return true;
    }

    if (req.method == "POST" && path == "/api/wasm/test-dispatch-click") {
        if (!IsWasmTestDispatchApiEnabled()) {
            SetPlainResponse(resp, 404, "not found");
            return true;
        }

        if (!controller || !controller->WasmHost()) {
            SetJsonResponse(resp, json({
                {"ok", false},
                {"error", "wasm host unavailable"},
            }).dump());
            return true;
        }

        const json payload = ParseObjectOrEmpty(req.body);
        wasm::EventInvokeInput invoke{};
        invoke.kind = wasm::EventKind::Click;
        invoke.x = ParseInt32OrDefault(payload, "x", 0);
        invoke.y = ParseInt32OrDefault(payload, "y", 0);
        invoke.button = ParseButtonOrDefault(payload, "button", 1);
        invoke.eventTickMs = controller->CurrentTickMs();

        wasm::WasmEffectHost* host = controller->WasmHost();
        const wasm::EventDispatchExecutionResult dispatchResult =
            wasm::InvokeEventAndRender(*host, invoke, controller->Config());
        const wasm::HostDiagnostics& diag = host->Diagnostics();

        SetJsonResponse(resp, json({
            {"ok", true},
            {"route_active", dispatchResult.routeActive},
            {"invoke_ok", dispatchResult.invokeOk},
            {"rendered_any", dispatchResult.render.renderedAny},
            {"executed_text_commands", dispatchResult.render.executedTextCommands},
            {"executed_image_commands", dispatchResult.render.executedImageCommands},
            {"throttled_commands", dispatchResult.render.throttledCommands},
            {"throttled_by_capacity_commands", dispatchResult.render.throttledByCapacityCommands},
            {"throttled_by_interval_commands", dispatchResult.render.throttledByIntervalCommands},
            {"dropped_commands", dispatchResult.render.droppedCommands},
            {"render_error", dispatchResult.render.lastError},
            {"plugin_loaded", diag.pluginLoaded},
            {"runtime_backend", diag.runtimeBackend},
            {"last_error", diag.lastError},
            {"last_render_error", diag.lastRenderError},
            {"last_output_bytes", diag.lastOutputBytes},
            {"last_command_count", diag.lastCommandCount},
        }).dump());
        return true;
    }

    return false;
}

} // namespace mousefx
