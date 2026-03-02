#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace mousefx::platform::scaffold {

struct WebUiAsset {
    std::string contentType;
    std::vector<uint8_t> bytes;
};

std::vector<std::filesystem::path> BuildWebUiBaseDirs();
bool TryLoadWebUiAsset(
    const std::vector<std::filesystem::path>& baseDirs,
    const std::string& requestPath,
    WebUiAsset* outAsset);
std::string BuildMissingWebUiMessage();

} // namespace mousefx::platform::scaffold
