#include "pch.h"

#include "Platform/posix/Shell/ScaffoldSettingsRouteConfig.h"
#include "Platform/posix/Shell/ScaffoldSettingsRouteCodec.h"

#include <cstdlib>
#include <string_view>

namespace mousefx::platform::scaffold {
namespace {

void ParseLoopbackUrlRoute(const std::string& url, SettingsRoute* route) {
    if (!route) {
        return;
    }

    constexpr std::string_view kPrefix = "http://127.0.0.1:";
    if (url.rfind(kPrefix, 0) != 0) {
        route->useEmbeddedServer = false;
        return;
    }

    const size_t portStart = kPrefix.size();
    const size_t slashPos = url.find('/', portStart);
    if (slashPos == std::string::npos) {
        route->useEmbeddedServer = false;
        return;
    }

    uint16_t parsedPort = 0;
    if (!TryParseUnsignedPort(std::string_view(url).substr(portStart, slashPos - portStart), &parsedPort)) {
        route->useEmbeddedServer = false;
        return;
    }

    route->port = parsedPort;
    const std::string pathWithQuery = url.substr(slashPos);
    route->path = NormalizePath(PathWithoutQuery(pathWithQuery));

    const std::string token = QueryValue(pathWithQuery, "token");
    if (!token.empty()) {
        route->token = token;
    }
}

} // namespace

SettingsRoute BuildSettingsRoute() {
    SettingsRoute route{};
    route.url = "http://127.0.0.1:9527/?token=scaffold";
    route.path = "/";

    const char* env = std::getenv("MFX_SCAFFOLD_SETTINGS_URL");
    if (!env || *env == '\0') {
        return route;
    }

    const std::string url(env);
    if (!IsSettingsUrlShellSafe(url)) {
        return route;
    }

    route.url = url;
    ParseLoopbackUrlRoute(url, &route);
    return route;
}

std::string NormalizePath(std::string path) {
    if (path.empty()) {
        return "/";
    }
    if (path.front() != '/') {
        path.insert(path.begin(), '/');
    }
    while (path.size() > 1 && path.back() == '/') {
        path.pop_back();
    }
    return path;
}

std::string PathWithoutQuery(const std::string& path) {
    const size_t queryPos = path.find('?');
    if (queryPos == std::string::npos) {
        return path;
    }
    return path.substr(0, queryPos);
}

std::string QueryValue(const std::string& path, const std::string& key) {
    const size_t queryPos = path.find('?');
    if (queryPos == std::string::npos) {
        return {};
    }

    const std::string query = path.substr(queryPos + 1);
    const std::string pattern = key + "=";
    size_t pos = 0;

    while (pos < query.size()) {
        const size_t ampPos = query.find('&', pos);
        const size_t end = (ampPos == std::string::npos) ? query.size() : ampPos;
        const std::string pair = query.substr(pos, end - pos);
        if (pair.rfind(pattern, 0) == 0) {
            return UrlDecodePercentCopy(pair.substr(pattern.size()));
        }
        if (ampPos == std::string::npos) {
            break;
        }
        pos = ampPos + 1;
    }
    return {};
}

std::string BuildTokenQuerySuffix(const std::string& token) {
    if (token.empty()) {
        return {};
    }
    return std::string("?token=") + UrlEncodeQueryValue(token);
}

bool IsHtmlPath(const std::string& pathOnly, const SettingsRoute& route) {
    const std::string normalized = NormalizePath(pathOnly);
    if (normalized == route.path) {
        return true;
    }
    if (normalized == "/" || normalized == "/index.html") {
        return true;
    }
    return false;
}

} // namespace mousefx::platform::scaffold
