#pragma once

#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace mousefx {

struct PetVisualAssetApplyResult {
    bool loaded{false};
    std::string loadedPath;
    std::string loadError;
};

std::string ResolvePetVisualModelSourceFormat(const std::string& loadedModelPath);

PetVisualAssetApplyResult ApplyPetVisualModelAsset(
    bool companionEnabled,
    bool visualHostActive,
    const std::vector<std::filesystem::path>& modelCandidates,
    const std::function<bool(const std::string&)>& loadModel);

PetVisualAssetApplyResult ApplyPetVisualSinglePathAsset(
    bool companionEnabled,
    bool visualHostActive,
    const std::filesystem::path& assetPath,
    const std::function<bool(const std::string&)>& loadAsset,
    const char* missingError,
    const char* unavailableError);

} // namespace mousefx
