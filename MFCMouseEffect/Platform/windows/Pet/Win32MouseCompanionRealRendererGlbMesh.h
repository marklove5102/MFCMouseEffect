#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace mousefx::windows {

struct Win32MouseCompanionRealRendererGlbMeshVertex final {
    float x{0.0f};
    float y{0.0f};
    float z{0.0f};
};

struct Win32MouseCompanionRealRendererGlbMeshTriangle final {
    std::array<Win32MouseCompanionRealRendererGlbMeshVertex, 3> vertices{};
    uint32_t materialIndex{0};
    uint32_t sourceNodeIndex{0};
};

struct Win32MouseCompanionRealRendererGlbMesh final {
    std::string sourcePath;
    std::string sourceFormat{"phase1_placeholder"};
    std::vector<Win32MouseCompanionRealRendererGlbMeshTriangle> triangles;
    float minX{0.0f};
    float minY{0.0f};
    float minZ{0.0f};
    float maxX{0.0f};
    float maxY{0.0f};
    float maxZ{0.0f};
    bool loaded{false};
};

Win32MouseCompanionRealRendererGlbMesh LoadWin32MouseCompanionRealRendererGlbMesh(
    const std::string& modelPath);

} // namespace mousefx::windows
