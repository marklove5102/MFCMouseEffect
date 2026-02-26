#include "pch.h"

#include "WasmPluginManifest.h"

#include "MouseFx/Core/Json/JsonFacade.h"
#include "MouseFx/Utils/StringUtils.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace mousefx::wasm {
namespace {

using json = nlohmann::json;

std::string ReadTextFileUtf8(const std::filesystem::path& filePath, std::string* outError) {
    std::ifstream input(filePath, std::ios::binary);
    if (!input.is_open()) {
        if (outError) {
            *outError = "Cannot open plugin manifest.";
        }
        return {};
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    if (input.bad()) {
        if (outError) {
            *outError = "Failed reading plugin manifest.";
        }
        return {};
    }
    return buffer.str();
}

bool TryReadStringField(const json& node, const char* key, std::string* outValue, std::string* outError) {
    if (!outValue) {
        return false;
    }
    if (!node.contains(key) || !node[key].is_string()) {
        if (outError) {
            *outError = std::string("Manifest field '") + key + "' must be string.";
        }
        return false;
    }
    *outValue = node[key].get<std::string>();
    return true;
}

bool TryReadUintField(const json& node, const char* key, uint32_t* outValue, std::string* outError) {
    if (!outValue) {
        return false;
    }
    if (!node.contains(key) || !node[key].is_number_unsigned()) {
        if (outError) {
            *outError = std::string("Manifest field '") + key + "' must be unsigned number.";
        }
        return false;
    }
    *outValue = node[key].get<uint32_t>();
    return true;
}

bool TryReadImageAssetsField(
    const json& node,
    std::vector<std::wstring>* outAssets,
    std::string* outError) {
    if (!outAssets) {
        return false;
    }
    outAssets->clear();
    if (!node.contains("image_assets")) {
        return true;
    }
    if (!node["image_assets"].is_array()) {
        if (outError) {
            *outError = "Manifest field 'image_assets' must be string array.";
        }
        return false;
    }
    for (const auto& item : node["image_assets"]) {
        if (!item.is_string()) {
            if (outError) {
                *outError = "Manifest field 'image_assets' must be string array.";
            }
            return false;
        }
        outAssets->push_back(Utf8ToWString(item.get<std::string>()));
    }
    return true;
}

} // namespace

PluginManifestLoadResult WasmPluginManifest::LoadFromFile(const std::wstring& manifestPath) {
    PluginManifestLoadResult result{};
    const std::filesystem::path path(manifestPath);
    std::error_code ec;
    if (!std::filesystem::exists(path, ec) || ec) {
        result.error = "Manifest file does not exist.";
        return result;
    }

    std::string readError;
    const std::string content = ReadTextFileUtf8(path, &readError);
    if (content.empty()) {
        result.error = readError.empty() ? "Manifest file is empty." : readError;
        return result;
    }

    json root;
    try {
        root = json::parse(content);
    } catch (const std::exception& ex) {
        result.error = std::string("Manifest JSON parse error: ") + ex.what();
        return result;
    }
    if (!root.is_object()) {
        result.error = "Manifest root must be a JSON object.";
        return result;
    }

    PluginManifest manifest{};
    if (!TryReadStringField(root, "id", &manifest.id, &result.error)) {
        return result;
    }
    if (!TryReadStringField(root, "name", &manifest.name, &result.error)) {
        return result;
    }
    if (!TryReadStringField(root, "version", &manifest.version, &result.error)) {
        return result;
    }
    if (!TryReadUintField(root, "api_version", &manifest.apiVersion, &result.error)) {
        return result;
    }

    std::string entryUtf8;
    if (!TryReadStringField(root, "entry", &entryUtf8, &result.error)) {
        return result;
    }
    manifest.entryWasm = Utf8ToWString(entryUtf8);
    if (!TryReadImageAssetsField(root, &manifest.imageAssets, &result.error)) {
        return result;
    }

    if (!Validate(manifest, &result.error)) {
        return result;
    }

    result.ok = true;
    result.manifest = std::move(manifest);
    return result;
}

} // namespace mousefx::wasm
