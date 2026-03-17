#include "pch.h"
#include "PetAssetFormat.h"

#include <algorithm>

namespace mousefx::pet {
namespace {

std::string ToLowerAscii(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        if (ch >= 'A' && ch <= 'Z') {
            return static_cast<char>(ch - 'A' + 'a');
        }
        return static_cast<char>(ch);
    });
    return value;
}

std::string ExtractExtension(const std::string& path) {
    const size_t slashPos = path.find_last_of("/\\");
    const size_t dotPos = path.find_last_of('.');
    if (dotPos == std::string::npos || (slashPos != std::string::npos && dotPos < slashPos)) {
        return {};
    }
    return ToLowerAscii(path.substr(dotPos + 1));
}

} // namespace

const char* ToString(ModelFormat format) {
    switch (format) {
    case ModelFormat::Glb:
        return "glb";
    case ModelFormat::Gltf:
        return "gltf";
    case ModelFormat::Usdz:
        return "usdz";
    case ModelFormat::Vrm:
        return "vrm";
    case ModelFormat::Fbx:
        return "fbx";
    default:
        return "unknown";
    }
}

ModelFormat DetectModelFormatFromPath(const std::string& path) {
    const std::string ext = ExtractExtension(path);
    if (ext == "glb") {
        return ModelFormat::Glb;
    }
    if (ext == "gltf") {
        return ModelFormat::Gltf;
    }
    if (ext == "usdz") {
        return ModelFormat::Usdz;
    }
    if (ext == "vrm") {
        return ModelFormat::Vrm;
    }
    if (ext == "fbx") {
        return ModelFormat::Fbx;
    }
    return ModelFormat::Unknown;
}

bool IsSupportedImportFormat(ModelFormat format) {
    switch (format) {
    case ModelFormat::Glb:
    case ModelFormat::Gltf:
    case ModelFormat::Usdz:
    case ModelFormat::Vrm:
    case ModelFormat::Fbx:
        return true;
    default:
        return false;
    }
}

bool IsCanonicalRuntimeFormat(ModelFormat format) {
    return format == ModelFormat::Glb;
}

} // namespace mousefx::pet
