#include "pch.h"
#include "WebSettingsServer.AutomationRoutes.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

#include "MouseFx/Core/Control/AppController.h"
#include "MouseFx/Core/Json/JsonFacade.h"
#include "MouseFx/Server/HttpServer.h"
#include "Platform/PlatformApplicationCatalog.h"

using json = nlohmann::json;

namespace mousefx {
namespace {

void SetJsonResponse(HttpResponse& resp, const std::string& body) {
    resp.statusCode = 200;
    resp.contentType = "application/json; charset=utf-8";
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

std::string ParseSessionId(const json& payload) {
    if (!payload.contains("session") || !payload["session"].is_string()) {
        return {};
    }
    return payload["session"].get<std::string>();
}

std::string PollStateToText(ShortcutCaptureSession::PollState state) {
    switch (state) {
    case ShortcutCaptureSession::PollState::Pending:
        return "pending";
    case ShortcutCaptureSession::PollState::Captured:
        return "captured";
    case ShortcutCaptureSession::PollState::Expired:
        return "expired";
    case ShortcutCaptureSession::PollState::InvalidSession:
    default:
        return "invalid";
    }
}

bool ParseForceRefresh(const json& payload) {
    if (!payload.contains("force")) {
        return false;
    }
    if (payload["force"].is_boolean()) {
        return payload["force"].get<bool>();
    }
    if (payload["force"].is_number_integer()) {
        return payload["force"].get<int>() != 0;
    }
    return false;
}

std::vector<platform::ApplicationCatalogEntry> LoadAutomationAppCatalog(bool forceRefresh) {
    static std::mutex cacheMutex;
    static uint64_t cacheTickMs = 0;
    static std::vector<platform::ApplicationCatalogEntry> cacheEntries;

    constexpr uint64_t kCacheTtlMs = 30 * 1000;
    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    const uint64_t nowTickMs = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(now).count());

    std::lock_guard<std::mutex> lock(cacheMutex);
    if (!forceRefresh &&
        !cacheEntries.empty() &&
        (nowTickMs - cacheTickMs) < kCacheTtlMs) {
        return cacheEntries;
    }

    cacheEntries = platform::ScanApplicationCatalog();
    cacheTickMs = nowTickMs;
    return cacheEntries;
}

} // namespace

bool HandleWebSettingsAutomationApiRoute(
    const HttpRequest& req,
    const std::string& path,
    AppController* controller,
    HttpResponse& resp) {
    if (req.method == "POST" && path == "/api/automation/shortcut-capture/start") {
        if (!controller) {
            SetJsonResponse(resp, json({{"ok", false}, {"error", "no controller"}}).dump());
            return true;
        }

        const json payload = ParseObjectOrEmpty(req.body);
        uint64_t timeoutMs = 10000;
        if (payload.contains("timeout_ms") && payload["timeout_ms"].is_number_integer()) {
            const int64_t value = payload["timeout_ms"].get<int64_t>();
            if (value > 0) {
                timeoutMs = static_cast<uint64_t>(value);
            }
        }
        timeoutMs = std::clamp<uint64_t>(timeoutMs, 1000, 30000);

        const std::string sessionId = controller->StartShortcutCaptureSession(timeoutMs);
        SetJsonResponse(resp, json({
            {"ok", !sessionId.empty()},
            {"session", sessionId}
        }).dump());
        return true;
    }

    if (req.method == "POST" && path == "/api/automation/shortcut-capture/poll") {
        if (!controller) {
            SetJsonResponse(resp, json({{"ok", false}, {"error", "no controller"}}).dump());
            return true;
        }

        const json payload = ParseObjectOrEmpty(req.body);
        const std::string sessionId = ParseSessionId(payload);
        const ShortcutCaptureSession::PollResult result = controller->PollShortcutCaptureSession(sessionId);

        json body{
            {"ok", true},
            {"status", PollStateToText(result.state)}
        };
        if (!result.shortcut.empty()) {
            body["shortcut"] = result.shortcut;
        }
        SetJsonResponse(resp, body.dump());
        return true;
    }

    if (req.method == "POST" && path == "/api/automation/shortcut-capture/stop") {
        if (!controller) {
            SetJsonResponse(resp, json({{"ok", false}, {"error", "no controller"}}).dump());
            return true;
        }

        const json payload = ParseObjectOrEmpty(req.body);
        const std::string sessionId = ParseSessionId(payload);
        controller->StopShortcutCaptureSession(sessionId);
        SetJsonResponse(resp, json({{"ok", true}}).dump());
        return true;
    }

    if (req.method == "POST" && path == "/api/automation/active-process") {
        const std::string processBaseName = controller
            ? controller->CurrentForegroundProcessBaseName()
            : std::string{};
        SetJsonResponse(resp, json({
            {"ok", true},
            {"process", processBaseName}
        }).dump());
        return true;
    }

    if (req.method == "POST" && path == "/api/automation/app-catalog") {
        const json payload = ParseObjectOrEmpty(req.body);
        const bool forceRefresh = ParseForceRefresh(payload);
        const std::vector<platform::ApplicationCatalogEntry> entries = LoadAutomationAppCatalog(forceRefresh);

        json apps = json::array();
        for (const auto& entry : entries) {
            apps.push_back({
                {"exe", entry.processName},
                {"label", entry.displayName},
                {"source", entry.source},
            });
        }

        SetJsonResponse(resp, json({
            {"ok", true},
            {"apps", apps},
            {"count", apps.size()},
        }).dump());
        return true;
    }

    return false;
}

} // namespace mousefx
