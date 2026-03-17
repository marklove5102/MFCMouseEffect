#pragma once

#include <string>

namespace mousefx::pet {

enum class ModelFormat : unsigned char {
    Unknown = 0,
    Glb = 1,
    Gltf = 2,
    Usdz = 3,
    Vrm = 4,
    Fbx = 5,
};

const char* ToString(ModelFormat format);
ModelFormat DetectModelFormatFromPath(const std::string& path);
bool IsSupportedImportFormat(ModelFormat format);
bool IsCanonicalRuntimeFormat(ModelFormat format);

} // namespace mousefx::pet
