#include "pch.h"
#include "MouseFx/Core/Pet/PetCanonicalModelValidator.h"

#include "MouseFx/Core/Json/JsonFacade.h"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <vector>

namespace mousefx::pet {
namespace {

constexpr uint32_t kGlbMagic = 0x46546C67u; // "glTF"
constexpr uint32_t kGlbVersion2 = 2u;
constexpr uint32_t kJsonChunkType = 0x4E4F534Au; // "JSON"

uint32_t ReadU32Le(const uint8_t* p) {
    return static_cast<uint32_t>(p[0]) |
           (static_cast<uint32_t>(p[1]) << 8) |
           (static_cast<uint32_t>(p[2]) << 16) |
           (static_cast<uint32_t>(p[3]) << 24);
}

bool ReadFileBytes(const std::string& path, std::vector<uint8_t>* outBytes) {
    if (!outBytes) {
        return false;
    }
    std::ifstream in(std::filesystem::path(path), std::ios::binary);
    if (!in) {
        return false;
    }
    in.seekg(0, std::ios::end);
    const std::streamoff size = in.tellg();
    if (size <= 0) {
        return false;
    }
    outBytes->assign(static_cast<size_t>(size), 0);
    in.seekg(0, std::ios::beg);
    in.read(reinterpret_cast<char*>(outBytes->data()), static_cast<std::streamsize>(outBytes->size()));
    return in.good() || in.eof();
}

std::string TrimJsonChunk(std::string jsonChunk) {
    while (!jsonChunk.empty()) {
        const char tail = jsonChunk.back();
        if (tail == '\0' || tail == '\n' || tail == '\r' || tail == '\t' || tail == ' ') {
            jsonChunk.pop_back();
            continue;
        }
        break;
    }
    return jsonChunk;
}

bool ExtractJsonChunk(const std::vector<uint8_t>& bytes, std::string* outJson, std::string* outError) {
    if (!outJson) {
        return false;
    }
    if (bytes.size() < 12) {
        if (outError) *outError = "glb file too small";
        return false;
    }
    const uint32_t magic = ReadU32Le(bytes.data());
    const uint32_t version = ReadU32Le(bytes.data() + 4);
    const uint32_t declaredLength = ReadU32Le(bytes.data() + 8);
    if (magic != kGlbMagic) {
        if (outError) *outError = "invalid glb magic";
        return false;
    }
    if (version != kGlbVersion2) {
        if (outError) *outError = "unsupported glb version";
        return false;
    }
    if (declaredLength > bytes.size() || declaredLength < 12) {
        if (outError) *outError = "invalid glb length";
        return false;
    }

    size_t offset = 12;
    while (offset + 8 <= declaredLength) {
        const uint32_t chunkLength = ReadU32Le(bytes.data() + offset);
        const uint32_t chunkType = ReadU32Le(bytes.data() + offset + 4);
        offset += 8;
        if (offset + chunkLength > declaredLength) {
            if (outError) *outError = "glb chunk overflow";
            return false;
        }
        if (chunkType == kJsonChunkType) {
            std::string jsonChunk(reinterpret_cast<const char*>(bytes.data() + offset), chunkLength);
            *outJson = TrimJsonChunk(std::move(jsonChunk));
            return !outJson->empty();
        }
        offset += chunkLength;
    }
    if (outError) *outError = "missing glb json chunk";
    return false;
}

int CountSkinJoints(const nlohmann::json& root) {
    const auto skinsIt = root.find("skins");
    if (skinsIt == root.end() || !skinsIt->is_array()) {
        return 0;
    }
    int total = 0;
    for (const auto& skin : *skinsIt) {
        const auto jointsIt = skin.find("joints");
        if (jointsIt == skin.end() || !jointsIt->is_array()) {
            continue;
        }
        total += static_cast<int>(jointsIt->size());
    }
    return total;
}

} // namespace

bool ValidateCanonicalGlb(const std::string& glbPath, CanonicalModelValidationReport* outReport) {
    if (!outReport) {
        return false;
    }

    CanonicalModelValidationReport report{};
    std::vector<uint8_t> bytes;
    if (!ReadFileBytes(glbPath, &bytes)) {
        report.error = "failed to read canonical glb";
        *outReport = std::move(report);
        return false;
    }

    std::string jsonText;
    if (!ExtractJsonChunk(bytes, &jsonText, &report.error)) {
        *outReport = std::move(report);
        return false;
    }

    nlohmann::json root;
    try {
        root = nlohmann::json::parse(jsonText);
    } catch (const nlohmann::json::exception& ex) {
        report.error = std::string("gltf json parse failed: ") + ex.what();
        *outReport = std::move(report);
        return false;
    }

    if (root.contains("nodes") && root["nodes"].is_array()) {
        report.nodeCount = static_cast<int>(root["nodes"].size());
    }
    if (root.contains("skins") && root["skins"].is_array()) {
        report.skinCount = static_cast<int>(root["skins"].size());
    }
    if (root.contains("materials") && root["materials"].is_array()) {
        report.materialCount = static_cast<int>(root["materials"].size());
    }
    if (root.contains("meshes") && root["meshes"].is_array()) {
        report.meshCount = static_cast<int>(root["meshes"].size());
    }
    report.jointCount = CountSkinJoints(root);

    if (report.nodeCount <= 0) {
        report.error = "canonical glb missing nodes";
        *outReport = std::move(report);
        return false;
    }
    if (report.meshCount <= 0) {
        report.error = "canonical glb missing meshes";
        *outReport = std::move(report);
        return false;
    }

    if (report.skinCount <= 0 || report.jointCount <= 0) {
        report.warnings.push_back("canonical glb has no skinned joints; animation reuse may be limited");
    }
    if (report.materialCount <= 0) {
        report.warnings.push_back("canonical glb has no materials; appearance overrides may be limited");
    }
    if (report.nodeCount > 1800) {
        report.warnings.push_back("canonical glb node count is high; runtime performance risk");
    }
    if (report.materialCount > 128) {
        report.warnings.push_back("canonical glb material count is high; appearance updates may be expensive");
    }

    report.ok = true;
    *outReport = std::move(report);
    return true;
}

} // namespace mousefx::pet

