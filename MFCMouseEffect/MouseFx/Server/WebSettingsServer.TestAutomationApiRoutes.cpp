#include "pch.h"
#include "WebSettingsServer.TestAutomationApiRoutes.h"

#include <chrono>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

#include "MouseFx/Core/Automation/AutomationActionIdNormalizer.h"
#include "MouseFx/Core/Automation/AppScopeUtils.h"
#include "MouseFx/Core/Automation/BindingMatchUtils.h"
#include "MouseFx/Core/Automation/ShortcutTextFormatter.h"
#include "MouseFx/Core/Control/AppController.h"
#include "MouseFx/Core/Protocol/InputTypes.h"
#include "MouseFx/Server/HttpServer.h"
#include "MouseFx/Server/WebSettingsServer.TestRouteCommon.h"
#include "MouseFx/Utils/StringUtils.h"
#include "Platform/macos/System/MacosVirtualKeyMapper.h"

using json = nlohmann::json;

namespace mousefx {
namespace {

using websettings_test_routes::IsEnabledByEnv;
using websettings_test_routes::ParseBooleanOrDefault;
using websettings_test_routes::ParseInt32OrDefault;
using websettings_test_routes::ParseObjectOrEmpty;
using websettings_test_routes::SetJsonResponse;
using websettings_test_routes::SetPlainResponse;

bool IsAutomationScopeTestApiEnabled() {
    return IsEnabledByEnv("MFX_ENABLE_AUTOMATION_SCOPE_TEST_API");
}

bool IsAutomationShortcutTestApiEnabled() {
    return IsEnabledByEnv("MFX_ENABLE_AUTOMATION_SHORTCUT_TEST_API");
}

bool IsAutomationInjectionTestApiEnabled() {
    return IsEnabledByEnv("MFX_ENABLE_AUTOMATION_INJECTION_TEST_API");
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

bool HandleWebSettingsTestAutomationApiRoute(
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

    return false;
}

} // namespace mousefx
