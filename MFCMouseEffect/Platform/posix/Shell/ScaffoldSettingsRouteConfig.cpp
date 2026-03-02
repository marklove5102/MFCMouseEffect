#include "pch.h"

#include "Platform/posix/Shell/ScaffoldSettingsRouteConfig.h"

#include <cstdlib>
#include <string_view>

namespace mousefx::platform::scaffold {
namespace {

bool IsSettingsUrlShellSafe(const std::string& url) {
    if (url.empty()) {
        return false;
    }
    for (char c : url) {
        if (c == '"' || c == '`' || c == '$' || c == '\n' || c == '\r') {
            return false;
        }
    }
    return true;
}

bool TryParseUnsignedPort(std::string_view text, uint16_t* outPort) {
    if (!outPort || text.empty() || text.size() > 5) {
        return false;
    }

    uint32_t value = 0;
    for (char c : text) {
        if (c < '0' || c > '9') {
            return false;
        }
        value = value * 10u + static_cast<uint32_t>(c - '0');
        if (value > 65535u) {
            return false;
        }
    }

    if (value == 0u) {
        return false;
    }
    *outPort = static_cast<uint16_t>(value);
    return true;
}

int HexDigitToInt(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return -1;
}

std::string UrlDecodePercentCopy(const std::string& value) {
    std::string out;
    out.reserve(value.size());
    for (size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '%' && i + 2 < value.size()) {
            const int hi = HexDigitToInt(value[i + 1]);
            const int lo = HexDigitToInt(value[i + 2]);
            if (hi >= 0 && lo >= 0) {
                out.push_back(static_cast<char>((hi << 4) | lo));
                i += 2;
                continue;
            }
        }
        out.push_back(value[i]);
    }
    return out;
}

std::string UrlEncodeQueryValue(const std::string& value) {
    static constexpr char kHex[] = "0123456789ABCDEF";
    std::string out;
    out.reserve(value.size() * 3);
    for (unsigned char c : value) {
        const bool isAlphaNum = (c >= 'a' && c <= 'z') ||
                                (c >= 'A' && c <= 'Z') ||
                                (c >= '0' && c <= '9');
        const bool isUnreserved = isAlphaNum || c == '-' || c == '_' || c == '.' || c == '~';
        if (isUnreserved) {
            out.push_back(static_cast<char>(c));
            continue;
        }
        out.push_back('%');
        out.push_back(kHex[(c >> 4) & 0x0f]);
        out.push_back(kHex[c & 0x0f]);
    }
    return out;
}

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
