#include "pch.h"
#include "MouseFx/Core/Pet/PetAppearanceProfile.h"

#include "MouseFx/Core/Json/JsonFacade.h"

#include <filesystem>
#include <fstream>
#include <sstream>

namespace mousefx::pet {
namespace {

bool ReadJsonFile(const std::string& path, std::string* outText) {
    if (!outText) {
        return false;
    }
    std::ifstream in(std::filesystem::path(path), std::ios::binary);
    if (!in) {
        return false;
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    *outText = ss.str();
    return !outText->empty();
}

void ParseStringArray(const nlohmann::json& source, const char* key, std::vector<std::string>* outValues) {
    if (!outValues) {
        return;
    }
    outValues->clear();
    const auto it = source.find(key);
    if (it == source.end() || !it->is_array()) {
        return;
    }
    outValues->reserve(it->size());
    for (const auto& value : *it) {
        if (value.is_string()) {
            outValues->push_back(value.get<std::string>());
        }
    }
}

AppearanceOverrides ParseAppearanceOverrides(const nlohmann::json& source) {
    AppearanceOverrides out{};
    const auto skinIt = source.find("skinVariantId");
    if (skinIt != source.end() && skinIt->is_string()) {
        out.skinVariantId = skinIt->get<std::string>();
    }
    ParseStringArray(source, "enabledAccessoryIds", &out.enabledAccessoryIds);
    ParseStringArray(source, "textureOverridePaths", &out.textureOverridePaths);
    if (out.textureOverridePaths.empty()) {
        ParseStringArray(source, "textureOverrides", &out.textureOverridePaths);
    }
    return out;
}

const nlohmann::json* ResolveProfileNode(const nlohmann::json& root) {
    const auto activePresetIt = root.find("activePreset");
    const auto presetsIt = root.find("presets");
    if (activePresetIt != root.end() && activePresetIt->is_string() &&
        presetsIt != root.end() && presetsIt->is_array()) {
        const std::string activePreset = activePresetIt->get<std::string>();
        for (const auto& preset : *presetsIt) {
            if (!preset.is_object()) {
                continue;
            }
            const auto idIt = preset.find("id");
            if (idIt != preset.end() && idIt->is_string() && idIt->get<std::string>() == activePreset) {
                return &preset;
            }
        }
    }

    const auto defaultIt = root.find("default");
    if (defaultIt != root.end() && defaultIt->is_object()) {
        return &(*defaultIt);
    }
    if (root.is_object()) {
        return &root;
    }
    return nullptr;
}

} // namespace

bool LoadPetAppearanceProfileFromJsonFile(const std::string& jsonPath,
                                          PetAppearanceProfile* outProfile,
                                          std::string* outError) {
    if (outError) {
        outError->clear();
    }
    if (!outProfile) {
        if (outError) {
            *outError = "appearance profile output pointer is null";
        }
        return false;
    }

    std::string text;
    if (!ReadJsonFile(jsonPath, &text)) {
        if (outError) {
            *outError = "failed to read appearance profile json";
        }
        return false;
    }

    nlohmann::json root;
    try {
        root = nlohmann::json::parse(text);
    } catch (const nlohmann::json::exception& ex) {
        if (outError) {
            *outError = std::string("appearance profile json parse failed: ") + ex.what();
        }
        return false;
    }

    const nlohmann::json* node = ResolveProfileNode(root);
    if (!node) {
        if (outError) {
            *outError = "appearance profile node not found";
        }
        return false;
    }

    PetAppearanceProfile profile{};
    profile.defaultAppearance = ParseAppearanceOverrides(*node);
    *outProfile = std::move(profile);
    return true;
}

} // namespace mousefx::pet

