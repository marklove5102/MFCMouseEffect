#include "pch.h"

#include "Platform/posix/Shell/ScaffoldSettingsRequestHandler.h"

#include "Platform/posix/Shell/ScaffoldSettingsWebUiAssets.h"

#include <utility>

namespace mousefx::platform::scaffold {

SettingsRequestHandler::SettingsRequestHandler(
    SettingsRoute route,
    std::vector<std::filesystem::path> webUiBaseDirs)
    : route_(std::move(route)),
      webUiBaseDirs_(std::move(webUiBaseDirs)) {
}

void SettingsRequestHandler::SetRuntimeMode(bool trayAvailable, bool backgroundMode) {
    std::lock_guard<std::mutex> lock(stateMutex_);
    trayAvailable_ = trayAvailable;
    backgroundMode_ = backgroundMode;
}

void SettingsRequestHandler::HandleRequest(const HttpRequest& req, HttpResponse& resp) {
    const std::string token = QueryValue(req.path, "token");
    if (!route_.token.empty() && token != route_.token) {
        SetPlainResponse(resp, 403, "forbidden");
        return;
    }

    const std::string pathOnly = NormalizePath(PathWithoutQuery(req.path));
    if (TryHandleApiRequest(req, pathOnly, resp)) {
        return;
    }
    if (TryHandleStaticRequest(req, pathOnly, resp)) {
        return;
    }

    if (req.method == "GET" && IsHtmlPath(pathOnly, route_)) {
        SetPlainResponse(resp, 503, BuildMissingWebUiMessage());
        return;
    }
    SetPlainResponse(resp, 404, "not found");
}

const SettingsRoute& SettingsRequestHandler::Route() const {
    return route_;
}

bool SettingsRequestHandler::HasWebUiBaseDir() const {
    return !webUiBaseDirs_.empty();
}

bool SettingsRequestHandler::TryHandleApiRequest(
    const HttpRequest& req,
    const std::string& pathOnly,
    HttpResponse& resp) {
    if (req.method == "GET" && pathOnly == "/api/health") {
        SetJsonResponse(resp, 200, BuildHealthJson(route_));
        return true;
    }
    if (req.method == "GET" && pathOnly == "/api/schema") {
        SetJsonResponse(resp, 200, BuildSchemaJson(route_));
        return true;
    }
    if (req.method == "GET" && pathOnly == "/api/state") {
        RuntimeState stateSnapshot;
        uint64_t revision = 0;
        bool tray = false;
        bool background = false;
        {
            std::lock_guard<std::mutex> lock(stateMutex_);
            stateSnapshot = runtimeState_;
            revision = runtimeStateRevision_;
            tray = trayAvailable_;
            background = backgroundMode_;
        }
        SetJsonResponse(resp, 200, BuildStateJson(route_, stateSnapshot, tray, background, revision));
        return true;
    }
    if (req.method == "POST" && pathOnly == "/api/state") {
        RuntimeState nextState;
        uint64_t revision = 0;
        bool tray = false;
        bool background = false;
        json error;
        {
            std::lock_guard<std::mutex> lock(stateMutex_);
            if (!ParseStatePatch(req.body, runtimeState_, &nextState, &error)) {
                SetJsonResponse(resp, 400, error);
                return true;
            }
            runtimeState_ = nextState;
            ++runtimeStateRevision_;
            revision = runtimeStateRevision_;
            tray = trayAvailable_;
            background = backgroundMode_;
        }
        SetJsonResponse(resp, 200, BuildStateJson(route_, nextState, tray, background, revision));
        return true;
    }

    return false;
}

bool SettingsRequestHandler::TryHandleStaticRequest(
    const HttpRequest& req,
    const std::string& pathOnly,
    HttpResponse& resp) {
    if (req.method != "GET") {
        return false;
    }
    if (pathOnly == "/favicon.ico") {
        resp.statusCode = 204;
        resp.contentType = "text/plain; charset=utf-8";
        resp.body.clear();
        return true;
    }

    std::string webPath = req.path;
    if (IsHtmlPath(pathOnly, route_)) {
        webPath = "/index.html";
    }

    WebUiAsset asset;
    if (!TryLoadWebUiAsset(webUiBaseDirs_, webPath, &asset)) {
        return false;
    }

    resp.statusCode = 200;
    resp.contentType = asset.contentType;
    resp.body.assign(reinterpret_cast<const char*>(asset.bytes.data()), asset.bytes.size());
    return true;
}

} // namespace mousefx::platform::scaffold
