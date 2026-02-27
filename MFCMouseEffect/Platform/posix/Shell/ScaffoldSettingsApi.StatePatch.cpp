#include "pch.h"

#include "Platform/posix/Shell/ScaffoldSettingsApi.h"
#include "MouseFx/Core/Config/EffectConfigInternal.h"

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
    std::string normalized;
    return config_internal::TryNormalizeHoldFollowMode(value, &normalized);
}

} // namespace

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
        next.holdFollowMode = config_internal::NormalizeHoldFollowMode(value);
    }

    *outState = std::move(next);
    return true;
}

} // namespace mousefx::platform::scaffold
