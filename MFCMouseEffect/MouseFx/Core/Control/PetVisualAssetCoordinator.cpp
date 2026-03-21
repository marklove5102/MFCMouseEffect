#include "pch.h"

#include "MouseFx/Core/Control/PetVisualAssetCoordinator.h"

#include "MouseFx/Utils/StringUtils.h"

namespace mousefx {

std::string ResolvePetVisualModelSourceFormat(const std::string& loadedModelPath) {
    if (loadedModelPath.empty()) {
        return "phase1_placeholder";
    }
    std::string ext =
        ToLowerAscii(TrimAscii(std::filesystem::path(loadedModelPath).extension().string()));
    if (!ext.empty() && ext[0] == '.') {
        ext.erase(0, 1);
    }
    return ext.empty() ? "phase1_placeholder" : ext;
}

PetVisualAssetApplyResult ApplyPetVisualModelAsset(
    bool companionEnabled,
    bool visualHostActive,
    const std::vector<std::filesystem::path>& modelCandidates,
    const std::function<bool(const std::string&)>& loadModel) {
    PetVisualAssetApplyResult result{};
    result.loadError = "phase1_visual_host_unavailable";

    if (!companionEnabled) {
        result.loadError = "mouse_companion_disabled";
        return result;
    }
    if (!visualHostActive || !loadModel) {
        return result;
    }

    for (const auto& candidate : modelCandidates) {
        if (candidate.empty()) {
            continue;
        }
        if (loadModel(candidate.string())) {
            result.loaded = true;
            result.loadedPath = candidate.string();
            result.loadError.clear();
            return result;
        }
    }

    result.loadError =
        modelCandidates.empty() ? "phase2_visual_model_missing" : "phase2_visual_model_fallback_placeholder";
    return result;
}

PetVisualAssetApplyResult ApplyPetVisualSinglePathAsset(
    bool companionEnabled,
    bool visualHostActive,
    const std::filesystem::path& assetPath,
    const std::function<bool(const std::string&)>& loadAsset,
    const char* missingError,
    const char* unavailableError) {
    PetVisualAssetApplyResult result{};
    result.loadError = "phase1_visual_host_unavailable";

    if (!companionEnabled) {
        result.loadError = "mouse_companion_disabled";
        return result;
    }
    if (!visualHostActive || !loadAsset) {
        return result;
    }
    if (assetPath.empty()) {
        result.loadError = missingError ? missingError : "asset_missing";
        return result;
    }
    if (!loadAsset(assetPath.string())) {
        result.loadError = unavailableError ? unavailableError : "asset_unavailable";
        return result;
    }

    result.loaded = true;
    result.loadedPath = assetPath.string();
    result.loadError.clear();
    return result;
}

} // namespace mousefx
