#include "pch.h"
#include "MouseFx/Core/Pet/PetInterfaces.h"

#include "MouseFx/Core/Json/JsonFacade.h"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace mousefx::pet {
namespace {

constexpr uint32_t kGlbMagic = 0x46546C67u; // "glTF"
constexpr uint32_t kGlbVersion2 = 2u;
constexpr uint32_t kJsonChunkType = 0x4E4F534Au; // "JSON"

void WriteU32Le(std::ostream& out, uint32_t value) {
    const uint8_t bytes[4] = {
        static_cast<uint8_t>(value & 0xFFu),
        static_cast<uint8_t>((value >> 8) & 0xFFu),
        static_cast<uint8_t>((value >> 16) & 0xFFu),
        static_cast<uint8_t>((value >> 24) & 0xFFu),
    };
    out.write(reinterpret_cast<const char*>(bytes), static_cast<std::streamsize>(sizeof(bytes)));
}

bool StartsWith(const std::string& value, const char* prefix) {
    if (!prefix) {
        return false;
    }
    const size_t prefixLen = std::char_traits<char>::length(prefix);
    if (value.size() < prefixLen) {
        return false;
    }
    return value.compare(0, prefixLen, prefix) == 0;
}

bool HasUriScheme(const std::string& uri) {
    const size_t colon = uri.find(':');
    if (colon == std::string::npos) {
        return false;
    }
    const size_t slash = uri.find('/');
    return slash == std::string::npos || colon < slash;
}

bool IsExternalRelativeUri(const std::string& uri) {
    if (uri.empty()) {
        return false;
    }
    if (StartsWith(uri, "data:")) {
        return false;
    }
    if (HasUriScheme(uri)) {
        return false;
    }
    if (uri[0] == '/' || uri[0] == '\\') {
        return false;
    }
    return true;
}

bool ContainsParentTraversal(const std::filesystem::path& path) {
    for (const auto& part : path) {
        if (part == "..") {
            return true;
        }
    }
    return false;
}

void CollectExternalUris(const nlohmann::json& arrayNode, std::set<std::string>* outUris) {
    if (!outUris || !arrayNode.is_array()) {
        return;
    }
    for (const auto& item : arrayNode) {
        const auto uriIt = item.find("uri");
        if (uriIt == item.end() || !uriIt->is_string()) {
            continue;
        }
        const std::string uri = uriIt->get<std::string>();
        if (IsExternalRelativeUri(uri)) {
            outUris->insert(uri);
        }
    }
}

bool CopyExternalResourceTree(const std::filesystem::path& sourceDir,
                              const std::filesystem::path& canonicalDir,
                              const std::set<std::string>& uris,
                              std::vector<std::string>* outWarnings) {
    std::error_code ec;
    for (const auto& uri : uris) {
        const std::filesystem::path relative = std::filesystem::path(uri).lexically_normal();
        if (relative.empty() || relative.is_absolute() || ContainsParentTraversal(relative)) {
            if (outWarnings) {
                outWarnings->push_back(std::string("converter.gltf.unsafe_uri: ") + uri);
            }
            return false;
        }

        const std::filesystem::path sourcePath = sourceDir / relative;
        if (!std::filesystem::exists(sourcePath)) {
            if (outWarnings) {
                outWarnings->push_back(std::string("converter.gltf.missing_resource: ") + sourcePath.string());
            }
            return false;
        }

        const std::filesystem::path targetPath = canonicalDir / relative;
        std::filesystem::create_directories(targetPath.parent_path(), ec);
        if (ec) {
            if (outWarnings) {
                outWarnings->push_back(std::string("converter.gltf.create_resource_dir_failed: ") + ec.message());
            }
            return false;
        }

        std::filesystem::copy_file(sourcePath, targetPath, std::filesystem::copy_options::overwrite_existing, ec);
        if (ec) {
            if (outWarnings) {
                outWarnings->push_back(std::string("converter.gltf.copy_resource_failed: ") + targetPath.string() + ": " + ec.message());
            }
            return false;
        }
    }

    return true;
}

bool IsCanonicalCacheUsable(const std::filesystem::path& canonicalPath,
                            const std::filesystem::path& sourcePath,
                            const std::filesystem::path& sourceDir,
                            const std::set<std::string>& uris) {
    std::error_code ec;
    if (!std::filesystem::exists(canonicalPath, ec) || ec) {
        return false;
    }

    const auto canonicalTime = std::filesystem::last_write_time(canonicalPath, ec);
    if (ec) {
        return false;
    }

    auto newestInput = std::filesystem::last_write_time(sourcePath, ec);
    if (ec) {
        return false;
    }

    for (const auto& uri : uris) {
        const std::filesystem::path dep = sourceDir / std::filesystem::path(uri).lexically_normal();
        const auto depTime = std::filesystem::last_write_time(dep, ec);
        if (ec) {
            return false;
        }
        if (depTime > newestInput) {
            newestInput = depTime;
        }
    }

    return canonicalTime >= newestInput;
}

bool WriteJsonOnlyGlb(const std::filesystem::path& outputPath,
                      const std::string& jsonText,
                      std::vector<std::string>* outWarnings) {
    std::vector<uint8_t> jsonBytes(jsonText.begin(), jsonText.end());
    while ((jsonBytes.size() % 4u) != 0u) {
        jsonBytes.push_back(static_cast<uint8_t>(' '));
    }

    const uint32_t jsonChunkLength = static_cast<uint32_t>(jsonBytes.size());
    const uint32_t totalLength = 12u + 8u + jsonChunkLength;

    std::ofstream out(outputPath, std::ios::binary | std::ios::trunc);
    if (!out) {
        if (outWarnings) {
            outWarnings->push_back(std::string("converter.gltf.write_open_failed: ") + outputPath.string());
        }
        return false;
    }

    WriteU32Le(out, kGlbMagic);
    WriteU32Le(out, kGlbVersion2);
    WriteU32Le(out, totalLength);
    WriteU32Le(out, jsonChunkLength);
    WriteU32Le(out, kJsonChunkType);
    if (!jsonBytes.empty()) {
        out.write(reinterpret_cast<const char*>(jsonBytes.data()), static_cast<std::streamsize>(jsonBytes.size()));
    }
    if (!out.good()) {
        if (outWarnings) {
            outWarnings->push_back(std::string("converter.gltf.write_failed: ") + outputPath.string());
        }
        return false;
    }
    return true;
}

class GltfJsonCanonicalFormatConverter final : public IModelFormatConverter {
public:
    bool Supports(ModelFormat sourceFormat) const override {
        return sourceFormat == ModelFormat::Gltf;
    }

    bool ConvertToCanonicalGlb(const std::string& sourcePath,
                               ModelFormat sourceFormat,
                               ModelConversionResult* outResult) override {
        if (!outResult) {
            return false;
        }
        *outResult = {};

        if (!Supports(sourceFormat)) {
            return false;
        }
        if (sourcePath.empty()) {
            outResult->warnings.push_back("converter.gltf.empty_source_path");
            return false;
        }

        const std::filesystem::path source = std::filesystem::path(sourcePath).lexically_normal();
        if (!std::filesystem::exists(source)) {
            outResult->warnings.push_back("converter.gltf.source_missing");
            return false;
        }

        std::ifstream in(source);
        if (!in) {
            outResult->warnings.push_back("converter.gltf.source_read_failed");
            return false;
        }
        const std::string jsonText((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        if (jsonText.empty()) {
            outResult->warnings.push_back("converter.gltf.source_empty");
            return false;
        }

        nlohmann::json root;
        try {
            root = nlohmann::json::parse(jsonText);
        } catch (const nlohmann::json::exception& ex) {
            outResult->warnings.push_back(std::string("converter.gltf.parse_failed: ") + ex.what());
            return false;
        }
        const auto assetIt = root.find("asset");
        if (assetIt == root.end() || !assetIt->is_object()) {
            outResult->warnings.push_back("converter.gltf.missing_asset_node");
            return false;
        }

        std::set<std::string> externalUris;
        const auto buffersIt = root.find("buffers");
        if (buffersIt != root.end()) {
            CollectExternalUris(*buffersIt, &externalUris);
        }
        const auto imagesIt = root.find("images");
        if (imagesIt != root.end()) {
            CollectExternalUris(*imagesIt, &externalUris);
        }

        const std::filesystem::path sourceDir = source.parent_path();
        const std::filesystem::path canonicalDir = sourceDir / "canonical";
        const std::filesystem::path canonicalPath = canonicalDir / (source.stem().string() + ".glb");

        std::error_code ec;
        std::filesystem::create_directories(canonicalDir, ec);
        if (ec) {
            outResult->warnings.push_back(std::string("converter.gltf.create_canonical_dir_failed: ") + ec.message());
            return false;
        }

        if (IsCanonicalCacheUsable(canonicalPath, source, sourceDir, externalUris)) {
            outResult->canonicalGlbPath = canonicalPath.string();
            outResult->converted = true;
            outResult->warnings.push_back(std::string("converter.gltf.reuse_cached_canonical: ") + canonicalPath.filename().string());
            return true;
        }

        if (!CopyExternalResourceTree(sourceDir, canonicalDir, externalUris, &outResult->warnings)) {
            return false;
        }

        const std::string canonicalJson = root.dump();
        if (!WriteJsonOnlyGlb(canonicalPath, canonicalJson, &outResult->warnings)) {
            return false;
        }

        outResult->canonicalGlbPath = canonicalPath.string();
        outResult->converted = true;
        outResult->warnings.push_back(std::string("converter.gltf.exported_canonical: ") + canonicalPath.filename().string());
        return true;
    }
};

} // namespace

std::unique_ptr<IModelFormatConverter> CreateGltfJsonCanonicalFormatConverter() {
    return std::make_unique<GltfJsonCanonicalFormatConverter>();
}

} // namespace mousefx::pet
