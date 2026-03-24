#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererModelMeshLayerBuilder.h"

#include <algorithm>

namespace mousefx::windows {
namespace {

struct ProjectedMeshTriangle final {
    Win32MouseCompanionRealRendererModelMeshTriangle triangle{};
    float depth{0.0f};
};

Gdiplus::Color ResolveMeshFillColor(uint32_t materialIndex) {
    switch (materialIndex % 5U) {
    case 0U:
        return Gdiplus::Color(118, 134, 220, 196);
    case 1U:
        return Gdiplus::Color(112, 246, 210, 158);
    case 2U:
        return Gdiplus::Color(104, 255, 194, 146);
    case 3U:
        return Gdiplus::Color(110, 164, 198, 255);
    default:
        return Gdiplus::Color(98, 255, 214, 168);
    }
}

Gdiplus::Color ScaleMeshColor(
    const Gdiplus::Color& color,
    float scale) {
    const auto scaleChannel = [scale](BYTE value) -> BYTE {
        return static_cast<BYTE>(std::clamp(static_cast<float>(value) * scale, 0.0f, 255.0f));
    };
    return Gdiplus::Color(
        color.GetA(),
        scaleChannel(color.GetR()),
        scaleChannel(color.GetG()),
        scaleChannel(color.GetB()));
}

Gdiplus::PointF ProjectMeshVertex(
    const Win32MouseCompanionRealRendererGlbMeshVertex& vertex,
    const Win32MouseCompanionRealRendererGlbMesh& mesh,
    const Win32MouseCompanionRealRendererScene& scene,
    float scale) {
    const float centerX = scene.centerX;
    const float centerY = scene.centerY + scene.bodyRect.Height * 0.10f;
    const float meshCenterX = (mesh.minX + mesh.maxX) * 0.5f;
    const float meshCenterY = (mesh.minY + mesh.maxY) * 0.5f;
    const float meshCenterZ = (mesh.minZ + mesh.maxZ) * 0.5f;
    const float x = vertex.x - meshCenterX;
    const float y = vertex.y - meshCenterY;
    const float z = vertex.z - meshCenterZ;
    return Gdiplus::PointF(
        centerX + x * scale + z * scale * 0.16f * scene.facingSign,
        centerY - y * scale + z * scale * 0.08f);
}

} // namespace

void BuildWin32MouseCompanionRealRendererModelMeshLayer(
    const Win32MouseCompanionRealRendererSceneRuntime& runtime,
    Win32MouseCompanionRealRendererScene& scene) {
    scene.modelMeshVisible = false;
    scene.modelMeshTriangles.clear();

    if (runtime.assets == nullptr || !runtime.assets->modelMeshLoaded || !runtime.assets->modelMesh.loaded) {
        return;
    }

    const auto& mesh = runtime.assets->modelMesh;
    const float extentX = std::max(1.0f, mesh.maxX - mesh.minX);
    const float extentY = std::max(1.0f, mesh.maxY - mesh.minY);
    const float extentZ = std::max(1.0f, mesh.maxZ - mesh.minZ);
    const float fitWidth = std::max(scene.bodyRect.Width * 1.7f, scene.headRect.Width * 2.4f);
    const float fitHeight = std::max(scene.bodyRect.Height * 1.8f, scene.headRect.Height * 2.8f);
    const float scale = std::min(
        fitWidth / (extentX + extentZ * 0.20f),
        fitHeight / (extentY + extentZ * 0.12f));

    std::vector<ProjectedMeshTriangle> projectedTriangles;
    projectedTriangles.reserve(mesh.triangles.size());
    for (const auto& triangle : mesh.triangles) {
        const float avgZ = (triangle.vertices[0].z + triangle.vertices[1].z + triangle.vertices[2].z) / 3.0f;
        const float depthNorm = extentZ <= 1.0f ? 0.5f : std::clamp((avgZ - mesh.minZ) / extentZ, 0.0f, 1.0f);
        const float shade = 0.70f + depthNorm * 0.42f;
        projectedTriangles.push_back(ProjectedMeshTriangle{
            Win32MouseCompanionRealRendererModelMeshTriangle{
                std::array<Gdiplus::PointF, 3>{
                    ProjectMeshVertex(triangle.vertices[0], mesh, scene, scale),
                    ProjectMeshVertex(triangle.vertices[1], mesh, scene, scale),
                    ProjectMeshVertex(triangle.vertices[2], mesh, scene, scale),
                },
                ScaleMeshColor(ResolveMeshFillColor(triangle.materialIndex), shade),
                std::clamp(116.0f + depthNorm * 72.0f + scene.proxyDominance * 28.0f, 112.0f, 210.0f),
            },
            avgZ,
        });
    }

    std::sort(
        projectedTriangles.begin(),
        projectedTriangles.end(),
        [](const ProjectedMeshTriangle& lhs, const ProjectedMeshTriangle& rhs) {
            return lhs.depth < rhs.depth;
        });

    scene.modelMeshTriangles.reserve(projectedTriangles.size());
    for (const auto& triangle : projectedTriangles) {
        scene.modelMeshTriangles.push_back(triangle.triangle);
    }

    scene.modelMeshVisible = !scene.modelMeshTriangles.empty();
}

} // namespace mousefx::windows
