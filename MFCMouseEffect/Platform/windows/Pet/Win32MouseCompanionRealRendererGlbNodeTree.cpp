#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererGlbNodeTree.h"

#include <fstream>
#include <functional>
#include <limits>
#include <nlohmann/json.hpp>

namespace mousefx::windows {
namespace {

using json = nlohmann::json;

uint32_t ReadUInt32LE(const std::vector<uint8_t>& data, size_t offset) {
    if (offset + 4 > data.size()) {
        return 0;
    }
    return static_cast<uint32_t>(data[offset]) |
        (static_cast<uint32_t>(data[offset + 1]) << 8U) |
        (static_cast<uint32_t>(data[offset + 2]) << 16U) |
        (static_cast<uint32_t>(data[offset + 3]) << 24U);
}

std::vector<uint8_t> ReadFileBytes(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return {};
    }
    input.seekg(0, std::ios::end);
    const std::streampos size = input.tellg();
    if (size <= 0) {
        return {};
    }
    std::vector<uint8_t> bytes(static_cast<size_t>(size));
    input.seekg(0, std::ios::beg);
    input.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (!input) {
        return {};
    }
    return bytes;
}

json ParseGlbRootJson(const std::vector<uint8_t>& bytes) {
    if (bytes.size() < 20) {
        return json::object();
    }
    const uint32_t magic = ReadUInt32LE(bytes, 0);
    const uint32_t version = ReadUInt32LE(bytes, 4);
    const uint32_t fileLength = ReadUInt32LE(bytes, 8);
    if (magic != 0x46546C67U || version != 2U || fileLength > bytes.size()) {
        return json::object();
    }

    const uint32_t jsonChunkLength = ReadUInt32LE(bytes, 12);
    const uint32_t jsonChunkType = ReadUInt32LE(bytes, 16);
    if (jsonChunkType != 0x4E4F534AU) {
        return json::object();
    }

    const size_t jsonStart = 20;
    const size_t jsonEnd = jsonStart + static_cast<size_t>(jsonChunkLength);
    if (jsonChunkLength == 0 || jsonEnd > bytes.size()) {
        return json::object();
    }

    try {
        return json::parse(bytes.begin() + static_cast<std::ptrdiff_t>(jsonStart),
                           bytes.begin() + static_cast<std::ptrdiff_t>(jsonEnd));
    } catch (const json::exception&) {
        return json::object();
    }
}

std::string ResolveNodeName(const json& nodeJson, uint32_t nodeIndex) {
    const std::string fallback = "node_" + std::to_string(nodeIndex);
    if (!nodeJson.is_object()) {
        return fallback;
    }
    const std::string name = nodeJson.value("name", std::string{});
    return name.empty() ? fallback : name;
}

std::vector<uint32_t> ResolveChildIndices(const json& nodeJson) {
    std::vector<uint32_t> children;
    if (!nodeJson.is_object() || !nodeJson.contains("children") || !nodeJson["children"].is_array()) {
        return children;
    }
    for (const json& child : nodeJson["children"]) {
        if (!child.is_number_unsigned()) {
            continue;
        }
        children.push_back(child.get<uint32_t>());
    }
    return children;
}

std::string JoinNodePath(
    const std::string& parentPath,
    const std::string& nodeName) {
    if (parentPath.empty()) {
        return "/" + nodeName;
    }
    return parentPath + "/" + nodeName;
}

} // namespace

Win32MouseCompanionRealRendererGlbNodeTree LoadWin32MouseCompanionRealRendererGlbNodeTree(
    const std::string& modelPath) {
    Win32MouseCompanionRealRendererGlbNodeTree tree{};
    tree.sourcePath = modelPath;
    tree.sourceFormat = "glb";

    if (modelPath.empty()) {
        return tree;
    }

    const std::vector<uint8_t> bytes = ReadFileBytes(modelPath);
    const json root = ParseGlbRootJson(bytes);
    if (!root.is_object() || !root.contains("nodes") || !root["nodes"].is_array()) {
        return tree;
    }

    const json& nodesJson = root["nodes"];
    const size_t nodeCount = nodesJson.size();
    if (nodeCount == 0 || nodeCount > static_cast<size_t>(std::numeric_limits<uint32_t>::max())) {
        return tree;
    }

    tree.nodes.resize(nodeCount);
    for (size_t i = 0; i < nodeCount; ++i) {
        Win32MouseCompanionRealRendererGlbNodeEntry& entry = tree.nodes[i];
        entry.nodeIndex = static_cast<uint32_t>(i);
        entry.nodeName = ResolveNodeName(nodesJson[i], entry.nodeIndex);
        entry.childIndices = ResolveChildIndices(nodesJson[i]);
    }

    for (size_t parentIndex = 0; parentIndex < nodeCount; ++parentIndex) {
        for (uint32_t childIndex : tree.nodes[parentIndex].childIndices) {
            if (childIndex >= tree.nodes.size()) {
                continue;
            }
            tree.nodes[childIndex].parentIndex = static_cast<int32_t>(parentIndex);
        }
    }

    std::function<void(uint32_t, const std::string&)> buildPaths =
        [&](uint32_t nodeIndex, const std::string& parentPath) {
            if (nodeIndex >= tree.nodes.size()) {
                return;
            }
            Win32MouseCompanionRealRendererGlbNodeEntry& entry = tree.nodes[nodeIndex];
            entry.nodePath = JoinNodePath(parentPath, entry.nodeName);
            for (uint32_t childIndex : entry.childIndices) {
                buildPaths(childIndex, entry.nodePath);
            }
        };

    bool rootAssigned = false;
    for (uint32_t i = 0; i < tree.nodes.size(); ++i) {
        if (tree.nodes[i].parentIndex >= 0) {
            continue;
        }
        if (!rootAssigned) {
            tree.rootNodeName = tree.nodes[i].nodeName;
            rootAssigned = true;
        }
        buildPaths(i, std::string{});
    }

    tree.loaded = rootAssigned;
    return tree;
}

} // namespace mousefx::windows
