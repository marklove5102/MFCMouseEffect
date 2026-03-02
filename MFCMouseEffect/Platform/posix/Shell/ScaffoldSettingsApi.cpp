#include "pch.h"

#include "Platform/posix/Shell/ScaffoldSettingsApi.h"

#include "Platform/PlatformTarget.h"

#include <utility>

namespace mousefx::platform::scaffold {
namespace {

bool IsSupportedUiLanguage(const std::string& value) {
    return value == "zh-CN" || value == "en-US";
}

bool IsSupportedTheme(const std::string& value) {
    return value == "scaffold";
}

bool IsSupportedHoldFollowMode(const std::string& value) {
    return value == "precise" || value == "smooth" || value == "efficient";
}

json BuildRouteJson(const SettingsRoute& route) {
    const std::string tokenQuery = BuildTokenQuerySuffix(route.token);
    return json{
        {"schema", std::string("/api/schema") + tokenQuery},
        {"state", std::string("/api/state") + tokenQuery},
        {"health", std::string("/api/health") + tokenQuery},
    };
}

const char* PlatformName() {
#if MFX_PLATFORM_MACOS
    return "macos";
#elif MFX_PLATFORM_LINUX
    return "linux";
#else
    return "unknown";
#endif
}

} // namespace

void SetJsonResponse(HttpResponse& resp, int statusCode, const json& payload) {
    resp.statusCode = statusCode;
    resp.contentType = "application/json; charset=utf-8";
    resp.body = payload.dump();
}

void SetPlainResponse(HttpResponse& resp, int statusCode, const std::string& body) {
    resp.statusCode = statusCode;
    resp.contentType = "text/plain; charset=utf-8";
    resp.body = body;
}

json BuildHealthJson(const SettingsRoute& route) {
    return json{
        {"ok", true},
        {"status", "ready"},
        {"service", "scaffold-settings"},
        {"platform", PlatformName()},
        {"entry_url", route.url},
        {"routes", BuildRouteJson(route)},
    };
}

json BuildSchemaJson(const SettingsRoute& route) {
    return json{
        {"ok", true},
        {"mode", "scaffold"},
        {"api_version", 1},
        {"token_required", !route.token.empty()},
        {"routes", BuildRouteJson(route)},
        {"capabilities", {
            {"read_state", true},
            {"write_state", true},
            {"apply_settings", false},
        }},
        {"fields", json::array({
            json{{"key", "ui_language"}, {"type", "select"}, {"default", "zh-CN"},
                 {"options", json::array({"zh-CN", "en-US"})}},
            json{{"key", "theme"}, {"type", "select"}, {"default", "scaffold"},
                 {"options", json::array({"scaffold"})}},
            json{{"key", "hold_follow_mode"}, {"type", "select"}, {"default", "precise"},
                 {"options", json::array({"precise", "smooth", "efficient"})}},
            json{{"key", "tray_mode"}, {"type", "readonly"}},
        })},
    };
}

json BuildStateJson(
    const SettingsRoute& route,
    const RuntimeState& state,
    bool trayAvailable,
    bool backgroundMode,
    uint64_t revision) {
    return json{
        {"ok", true},
        {"mode", "scaffold"},
        {"platform", PlatformName()},
        {"entry_url", route.url},
        {"entry_path", route.path},
        {"token_required", !route.token.empty()},
        {"tray_available", trayAvailable},
        {"runtime_mode", backgroundMode ? "background" : "tray"},
        {"state_revision", revision},
        {"state", {
            {"ui_language", state.uiLanguage},
            {"theme", state.theme},
            {"hold_follow_mode", state.holdFollowMode},
        }},
        {"write_enabled", true},
    };
}

json BuildErrorJson(const std::string& code, const std::string& message) {
    return json{{"ok", false}, {"error", code}, {"message", message}};
}

bool ParseStatePatch(
    const std::string& body,
    const RuntimeState& current,
    RuntimeState* outState,
    json* outError) {
    if (!outState) {
        if (outError) {
            *outError = BuildErrorJson("internal_error", "state container missing");
        }
        return false;
    }

    json payload;
    try {
        payload = json::parse(body.empty() ? std::string("{}") : body);
    } catch (...) {
        if (outError) {
            *outError = BuildErrorJson("invalid_json", "request body is not valid JSON");
        }
        return false;
    }
    if (!payload.is_object()) {
        if (outError) {
            *outError = BuildErrorJson("invalid_payload", "request body must be a JSON object");
        }
        return false;
    }

    RuntimeState next = current;

    if (payload.contains("ui_language")) {
        if (!payload["ui_language"].is_string()) {
            if (outError) {
                *outError = BuildErrorJson("invalid_ui_language", "ui_language must be string");
            }
            return false;
        }
        const std::string value = payload["ui_language"].get<std::string>();
        if (!IsSupportedUiLanguage(value)) {
            if (outError) {
                *outError = BuildErrorJson("invalid_ui_language", "ui_language must be zh-CN or en-US");
            }
            return false;
        }
        next.uiLanguage = value;
    }

    if (payload.contains("theme")) {
        if (!payload["theme"].is_string()) {
            if (outError) {
                *outError = BuildErrorJson("invalid_theme", "theme must be string");
            }
            return false;
        }
        const std::string value = payload["theme"].get<std::string>();
        if (!IsSupportedTheme(value)) {
            if (outError) {
                *outError = BuildErrorJson("invalid_theme", "theme must be scaffold");
            }
            return false;
        }
        next.theme = value;
    }

    if (payload.contains("hold_follow_mode")) {
        if (!payload["hold_follow_mode"].is_string()) {
            if (outError) {
                *outError = BuildErrorJson("invalid_hold_follow_mode", "hold_follow_mode must be string");
            }
            return false;
        }
        const std::string value = payload["hold_follow_mode"].get<std::string>();
        if (!IsSupportedHoldFollowMode(value)) {
            if (outError) {
                *outError = BuildErrorJson(
                    "invalid_hold_follow_mode",
                    "hold_follow_mode must be precise, smooth, or efficient");
            }
            return false;
        }
        next.holdFollowMode = value;
    }

    *outState = std::move(next);
    return true;
}

} // namespace mousefx::platform::scaffold
