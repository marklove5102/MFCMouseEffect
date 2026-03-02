#pragma once

#include <cstdint>
#include <string>

namespace mousefx::platform::scaffold {

struct SettingsRoute {
    std::string url{};
    std::string path = "/";
    uint16_t port = 9527;
    std::string token = "scaffold";
    bool useEmbeddedServer = true;
};

SettingsRoute BuildSettingsRoute();

std::string NormalizePath(std::string path);
std::string PathWithoutQuery(const std::string& path);
std::string QueryValue(const std::string& path, const std::string& key);
std::string BuildTokenQuerySuffix(const std::string& token);
bool IsHtmlPath(const std::string& pathOnly, const SettingsRoute& route);

} // namespace mousefx::platform::scaffold
