#include "pch.h"
#include "MouseFx/Core/Pet/PetGlbSkeletonLoader.h"

#include "MouseFx/Core/Json/JsonFacade.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <unordered_map>
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
        if (tail == '\0' || std::isspace(static_cast<unsigned char>(tail))) {
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

std::string DefaultBoneName(int nodeIndex) {
    return "joint_" + std::to_string(nodeIndex);
}

bool ParseSkeletonFromJson(const nlohmann::json& root, const std::string& modelName, SkeletonDesc* outSkeleton, std::string* outError) {
    if (!outSkeleton) {
        return false;
    }
    outSkeleton->name = modelName;
    outSkeleton->bones.clear();

    if (!root.contains("nodes") || !root["nodes"].is_array()) {
        if (outError) *outError = "gltf missing nodes";
        return false;
    }
    const nlohmann::json& nodes = root["nodes"];
    std::vector<int> parentNode(nodes.size(), -1);
    for (size_t i = 0; i < nodes.size(); ++i) {
        const nlohmann::json& node = nodes[i];
        const auto childIt = node.find("children");
        if (childIt == node.end() || !childIt->is_array()) {
            continue;
        }
        for (const auto& child : *childIt) {
            if (!child.is_number_integer()) {
                continue;
            }
            const int childIndex = child.get<int>();
            if (childIndex >= 0 && static_cast<size_t>(childIndex) < nodes.size()) {
                parentNode[static_cast<size_t>(childIndex)] = static_cast<int>(i);
            }
        }
    }

    std::vector<int> jointNodes;
    const auto skinsIt = root.find("skins");
    if (skinsIt != root.end() && skinsIt->is_array()) {
        for (const auto& skin : *skinsIt) {
            const auto jointsIt = skin.find("joints");
            if (jointsIt == skin.end() || !jointsIt->is_array() || jointsIt->empty()) {
                continue;
            }
            for (const auto& joint : *jointsIt) {
                if (joint.is_number_integer()) {
                    jointNodes.push_back(joint.get<int>());
                }
            }
            if (!jointNodes.empty()) {
                break;
            }
        }
    }

    if (jointNodes.empty()) {
        for (size_t i = 0; i < nodes.size(); ++i) {
            if (nodes[i].contains("name")) {
                jointNodes.push_back(static_cast<int>(i));
            }
        }
    }

    if (jointNodes.empty()) {
        if (outError) *outError = "no joints/nodes available for skeleton";
        return false;
    }

    std::unordered_map<int, int> nodeToBone;
    for (int nodeIndex : jointNodes) {
        if (nodeIndex < 0 || static_cast<size_t>(nodeIndex) >= nodes.size()) {
            continue;
        }
        const int boneIndex = static_cast<int>(outSkeleton->bones.size());
        nodeToBone[nodeIndex] = boneIndex;

        SkeletonBone bone{};
        bone.sourceNodeIndex = nodeIndex;
        const nlohmann::json& node = nodes[static_cast<size_t>(nodeIndex)];
        const auto nameIt = node.find("name");
        if (nameIt != node.end() && nameIt->is_string() && !nameIt->get_ref<const std::string&>().empty()) {
            bone.name = nameIt->get<std::string>();
        } else {
            bone.name = DefaultBoneName(nodeIndex);
        }
        outSkeleton->bones.push_back(std::move(bone));
    }

    for (auto& bone : outSkeleton->bones) {
        const int parentNodeIndex = parentNode[static_cast<size_t>(bone.sourceNodeIndex)];
        const auto mapped = nodeToBone.find(parentNodeIndex);
        bone.parentIndex = (mapped == nodeToBone.end()) ? -1 : mapped->second;
    }

    return !outSkeleton->bones.empty();
}

} // namespace

bool LoadSkeletonFromGlb(const std::string& glbPath, SkeletonDesc* outSkeleton, std::string* outError) {
    if (outError) {
        outError->clear();
    }
    std::vector<uint8_t> bytes;
    if (!ReadFileBytes(glbPath, &bytes)) {
        if (outError) *outError = "failed to read glb file";
        return false;
    }

    std::string jsonText;
    if (!ExtractJsonChunk(bytes, &jsonText, outError)) {
        return false;
    }

    nlohmann::json root;
    try {
        root = nlohmann::json::parse(jsonText);
    } catch (const nlohmann::json::exception& ex) {
        if (outError) *outError = std::string("gltf json parse failed: ") + ex.what();
        return false;
    }

    const std::string modelName = std::filesystem::path(glbPath).stem().string();
    return ParseSkeletonFromJson(root, modelName, outSkeleton, outError);
}

} // namespace mousefx::pet
