#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererGlbMesh.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <limits>

#include "MouseFx/ThirdParty/json.hpp"

namespace mousefx::windows {
namespace {

using json = nlohmann::json;

constexpr uint32_t kGlbMagic = 0x46546C67U;
constexpr uint32_t kGlbVersion = 2U;
constexpr uint32_t kJsonChunkType = 0x4E4F534AU;
constexpr uint32_t kBinChunkType = 0x004E4942U;
constexpr size_t kTriangleBudget = 2200;

struct GlbAccessorSpan final {
    size_t offset{0};
    size_t stride{0};
    size_t count{0};
    uint32_t componentType{0};
    std::string type;
    bool valid{false};
};

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

bool ParseGlbChunks(
    const std::vector<uint8_t>& bytes,
    json* rootJson,
    const uint8_t** binData,
    size_t* binSize) {
    if (rootJson == nullptr || binData == nullptr || binSize == nullptr || bytes.size() < 20) {
        return false;
    }

    const uint32_t magic = ReadUInt32LE(bytes, 0);
    const uint32_t version = ReadUInt32LE(bytes, 4);
    const uint32_t fileLength = ReadUInt32LE(bytes, 8);
    if (magic != kGlbMagic || version != kGlbVersion || fileLength > bytes.size()) {
        return false;
    }

    const uint32_t jsonChunkLength = ReadUInt32LE(bytes, 12);
    const uint32_t jsonChunkType = ReadUInt32LE(bytes, 16);
    if (jsonChunkType != kJsonChunkType) {
        return false;
    }

    const size_t jsonStart = 20;
    const size_t jsonEnd = jsonStart + static_cast<size_t>(jsonChunkLength);
    if (jsonChunkLength == 0 || jsonEnd > bytes.size()) {
        return false;
    }

    try {
        *rootJson = json::parse(
            bytes.begin() + static_cast<std::ptrdiff_t>(jsonStart),
            bytes.begin() + static_cast<std::ptrdiff_t>(jsonEnd));
    } catch (const json::exception&) {
        return false;
    }

    size_t cursor = jsonEnd;
    while (cursor + 8 <= bytes.size()) {
        const uint32_t chunkLength = ReadUInt32LE(bytes, cursor);
        const uint32_t chunkType = ReadUInt32LE(bytes, cursor + 4);
        cursor += 8;
        const size_t chunkEnd = cursor + static_cast<size_t>(chunkLength);
        if (chunkEnd > bytes.size()) {
            return false;
        }
        if (chunkType == kBinChunkType && chunkLength > 0) {
            *binData = bytes.data() + cursor;
            *binSize = static_cast<size_t>(chunkLength);
            return true;
        }
        cursor = chunkEnd;
    }
    return false;
}

size_t ComponentSizeOf(uint32_t componentType) {
    switch (componentType) {
    case 5121U:
        return 1;
    case 5123U:
        return 2;
    case 5125U:
    case 5126U:
        return 4;
    default:
        return 0;
    }
}

size_t ComponentCountOf(const std::string& type) {
    if (type == "SCALAR") {
        return 1;
    }
    if (type == "VEC2") {
        return 2;
    }
    if (type == "VEC3") {
        return 3;
    }
    if (type == "VEC4") {
        return 4;
    }
    return 0;
}

GlbAccessorSpan ResolveAccessorSpan(
    const json& root,
    const uint8_t* binData,
    size_t binSize,
    int accessorIndex) {
    GlbAccessorSpan span{};
    if (accessorIndex < 0 ||
        !root.contains("accessors") ||
        !root["accessors"].is_array() ||
        !root.contains("bufferViews") ||
        !root["bufferViews"].is_array()) {
        return span;
    }

    const json& accessors = root["accessors"];
    const json& bufferViews = root["bufferViews"];
    if (accessorIndex >= accessors.size()) {
        return span;
    }

    const json& accessor = accessors[accessorIndex];
    if (!accessor.is_object() ||
        !accessor.contains("bufferView") ||
        !accessor["bufferView"].is_number_integer()) {
        return span;
    }

    const int bufferViewIndex = accessor["bufferView"].get<int>();
    if (bufferViewIndex < 0 || bufferViewIndex >= bufferViews.size()) {
        return span;
    }

    const json& bufferView = bufferViews[bufferViewIndex];
    if (!bufferView.is_object()) {
        return span;
    }

    const uint32_t componentType = accessor.value("componentType", 0U);
    const std::string type = accessor.value("type", std::string{});
    const size_t componentSize = ComponentSizeOf(componentType);
    const size_t componentCount = ComponentCountOf(type);
    const size_t count = accessor.value("count", 0U);
    if (componentSize == 0 || componentCount == 0 || count == 0) {
        return span;
    }

    const size_t elementSize = componentSize * componentCount;
    const size_t accessorOffset = accessor.value("byteOffset", 0U);
    const size_t bufferViewOffset = bufferView.value("byteOffset", 0U);
    const size_t bufferViewLength = bufferView.value("byteLength", 0U);
    const size_t stride = std::max(
        elementSize,
        static_cast<size_t>(bufferView.value("byteStride", static_cast<uint32_t>(elementSize))));
    const size_t offset = bufferViewOffset + accessorOffset;

    if (offset >= binSize || stride == 0) {
        return span;
    }
    if (bufferViewLength < accessorOffset + (count - 1U) * stride + elementSize) {
        return span;
    }
    if (offset + (count - 1U) * stride + elementSize > binSize) {
        return span;
    }

    span.offset = offset;
    span.stride = stride;
    span.count = count;
    span.componentType = componentType;
    span.type = type;
    span.valid = binData != nullptr;
    return span;
}

bool ReadFloat32(
    const uint8_t* data,
    size_t size,
    size_t offset,
    float* outValue) {
    if (outValue == nullptr || offset + sizeof(float) > size) {
        return false;
    }
    std::memcpy(outValue, data + offset, sizeof(float));
    return true;
}

bool ReadUIntFromAccessor(
    const uint8_t* data,
    size_t size,
    const GlbAccessorSpan& span,
    size_t index,
    uint32_t* outValue) {
    if (!span.valid || outValue == nullptr || index >= span.count) {
        return false;
    }

    const size_t valueOffset = span.offset + index * span.stride;
    if (valueOffset >= size) {
        return false;
    }

    switch (span.componentType) {
    case 5121U:
        if (valueOffset + 1 > size) {
            return false;
        }
        *outValue = data[valueOffset];
        return true;
    case 5123U:
        if (valueOffset + 2 > size) {
            return false;
        }
        *outValue = static_cast<uint32_t>(data[valueOffset]) |
            (static_cast<uint32_t>(data[valueOffset + 1]) << 8U);
        return true;
    case 5125U:
        if (valueOffset + 4 > size) {
            return false;
        }
        *outValue = static_cast<uint32_t>(data[valueOffset]) |
            (static_cast<uint32_t>(data[valueOffset + 1]) << 8U) |
            (static_cast<uint32_t>(data[valueOffset + 2]) << 16U) |
            (static_cast<uint32_t>(data[valueOffset + 3]) << 24U);
        return true;
    default:
        return false;
    }
}

bool ReadVertexFromAccessor(
    const uint8_t* data,
    size_t size,
    const GlbAccessorSpan& span,
    size_t index,
    Win32MouseCompanionRealRendererGlbMeshVertex* outVertex) {
    if (!span.valid ||
        outVertex == nullptr ||
        span.componentType != 5126U ||
        span.type != "VEC3" ||
        index >= span.count) {
        return false;
    }

    const size_t vertexOffset = span.offset + index * span.stride;
    return ReadFloat32(data, size, vertexOffset, &outVertex->x) &&
        ReadFloat32(data, size, vertexOffset + 4, &outVertex->y) &&
        ReadFloat32(data, size, vertexOffset + 8, &outVertex->z);
}

size_t ResolvePrimitiveTriangleCount(
    const uint8_t* binData,
    size_t binSize,
    const json& root,
    const json& primitive) {
    if (!primitive.is_object() || primitive.value("mode", 4U) != 4U) {
        return 0;
    }
    if (!primitive.contains("attributes") || !primitive["attributes"].is_object()) {
        return 0;
    }
    const json& attributes = primitive["attributes"];
    if (!attributes.contains("POSITION") || !attributes["POSITION"].is_number_integer()) {
        return 0;
    }

    const GlbAccessorSpan positionSpan = ResolveAccessorSpan(
        root,
        binData,
        binSize,
        attributes["POSITION"].get<int>());
    if (!positionSpan.valid) {
        return 0;
    }

    if (primitive.contains("indices") && primitive["indices"].is_number_integer()) {
        const GlbAccessorSpan indexSpan = ResolveAccessorSpan(
            root,
            binData,
            binSize,
            primitive["indices"].get<int>());
        if (!indexSpan.valid || indexSpan.type != "SCALAR") {
            return 0;
        }
        return indexSpan.count / 3U;
    }

    return positionSpan.count / 3U;
}

void UpdateBounds(
    const Win32MouseCompanionRealRendererGlbMeshTriangle& triangle,
    Win32MouseCompanionRealRendererGlbMesh* mesh,
    bool* boundsInitialized) {
    if (mesh == nullptr || boundsInitialized == nullptr) {
        return;
    }
    for (const auto& vertex : triangle.vertices) {
        if (!*boundsInitialized) {
            mesh->minX = mesh->maxX = vertex.x;
            mesh->minY = mesh->maxY = vertex.y;
            mesh->minZ = mesh->maxZ = vertex.z;
            *boundsInitialized = true;
            continue;
        }
        mesh->minX = std::min(mesh->minX, vertex.x);
        mesh->minY = std::min(mesh->minY, vertex.y);
        mesh->minZ = std::min(mesh->minZ, vertex.z);
        mesh->maxX = std::max(mesh->maxX, vertex.x);
        mesh->maxY = std::max(mesh->maxY, vertex.y);
        mesh->maxZ = std::max(mesh->maxZ, vertex.z);
    }
}

} // namespace

Win32MouseCompanionRealRendererGlbMesh LoadWin32MouseCompanionRealRendererGlbMesh(
    const std::string& modelPath) {
    Win32MouseCompanionRealRendererGlbMesh mesh{};
    mesh.sourcePath = modelPath;
    mesh.sourceFormat = "glb";

    if (modelPath.empty()) {
        return mesh;
    }

    const std::vector<uint8_t> bytes = ReadFileBytes(modelPath);
    json root = json::object();
    const uint8_t* binData = nullptr;
    size_t binSize = 0;
    if (!ParseGlbChunks(bytes, &root, &binData, &binSize) ||
        !root.contains("meshes") ||
        !root["meshes"].is_array()) {
        return mesh;
    }

    size_t totalTriangleCount = 0;
    for (const json& meshJson : root["meshes"]) {
        if (!meshJson.is_object() || !meshJson.contains("primitives") || !meshJson["primitives"].is_array()) {
            continue;
        }
        for (const json& primitive : meshJson["primitives"]) {
            totalTriangleCount += ResolvePrimitiveTriangleCount(binData, binSize, root, primitive);
        }
    }

    const size_t sampleStep = std::max<size_t>(
        1U,
        totalTriangleCount == 0 ? 1U : ((totalTriangleCount + kTriangleBudget - 1U) / kTriangleBudget));

    std::vector<uint32_t> meshToNodeIndex;
    if (root.contains("nodes") && root["nodes"].is_array()) {
        meshToNodeIndex.resize(root["meshes"].size(), 0U);
        for (size_t nodeIndex = 0; nodeIndex < root["nodes"].size(); ++nodeIndex) {
            const json& node = root["nodes"][nodeIndex];
            if (!node.is_object() || !node.contains("mesh") || !node["mesh"].is_number_integer()) {
                continue;
            }
            const int meshIndex = node["mesh"].get<int>();
            if (meshIndex < 0 || static_cast<size_t>(meshIndex) >= meshToNodeIndex.size()) {
                continue;
            }
            meshToNodeIndex[static_cast<size_t>(meshIndex)] = static_cast<uint32_t>(nodeIndex);
        }
    }

    bool boundsInitialized = false;
    mesh.triangles.reserve(std::min(totalTriangleCount, kTriangleBudget));
    for (size_t meshIndex = 0; meshIndex < root["meshes"].size(); ++meshIndex) {
        const json& meshJson = root["meshes"][meshIndex];
        if (!meshJson.is_object() || !meshJson.contains("primitives") || !meshJson["primitives"].is_array()) {
            continue;
        }

        const uint32_t sourceNodeIndex =
            meshIndex < meshToNodeIndex.size() ? meshToNodeIndex[meshIndex] : 0U;
        for (const json& primitive : meshJson["primitives"]) {
            if (!primitive.is_object() || primitive.value("mode", 4U) != 4U) {
                continue;
            }
            if (!primitive.contains("attributes") || !primitive["attributes"].is_object()) {
                continue;
            }
            const json& attributes = primitive["attributes"];
            if (!attributes.contains("POSITION") || !attributes["POSITION"].is_number_integer()) {
                continue;
            }

            const GlbAccessorSpan positionSpan = ResolveAccessorSpan(
                root,
                binData,
                binSize,
                attributes["POSITION"].get<int>());
            if (!positionSpan.valid || positionSpan.componentType != 5126U || positionSpan.type != "VEC3") {
                continue;
            }

            GlbAccessorSpan indexSpan{};
            const bool indexed = primitive.contains("indices") && primitive["indices"].is_number_integer();
            if (indexed) {
                indexSpan = ResolveAccessorSpan(root, binData, binSize, primitive["indices"].get<int>());
                if (!indexSpan.valid || indexSpan.type != "SCALAR") {
                    continue;
                }
            }

            const uint32_t materialIndex = primitive.value("material", 0U);
            const size_t triangleCount = indexed ? indexSpan.count / 3U : positionSpan.count / 3U;
            for (size_t triangleIndex = 0; triangleIndex < triangleCount; ++triangleIndex) {
                if (triangleIndex % sampleStep != 0U) {
                    continue;
                }

                Win32MouseCompanionRealRendererGlbMeshTriangle triangle{};
                triangle.materialIndex = materialIndex;
                triangle.sourceNodeIndex = sourceNodeIndex;

                bool triangleValid = true;
                for (size_t vertexIndex = 0; vertexIndex < 3; ++vertexIndex) {
                    size_t positionIndex = triangleIndex * 3U + vertexIndex;
                    if (indexed) {
                        uint32_t resolvedIndex = 0U;
                        if (!ReadUIntFromAccessor(
                                binData,
                                binSize,
                                indexSpan,
                                triangleIndex * 3U + vertexIndex,
                                &resolvedIndex)) {
                            triangleValid = false;
                            break;
                        }
                        positionIndex = static_cast<size_t>(resolvedIndex);
                    }

                    if (!ReadVertexFromAccessor(
                            binData,
                            binSize,
                            positionSpan,
                            positionIndex,
                            &triangle.vertices[vertexIndex])) {
                        triangleValid = false;
                        break;
                    }
                }

                if (!triangleValid) {
                    continue;
                }

                UpdateBounds(triangle, &mesh, &boundsInitialized);
                mesh.triangles.push_back(triangle);
            }
        }
    }

    mesh.loaded = !mesh.triangles.empty() && boundsInitialized;
    return mesh;
}

} // namespace mousefx::windows
