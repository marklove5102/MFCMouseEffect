#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererModelSceneGraphBuilder.h"

#include <algorithm>
#include <array>
#include <unordered_map>

namespace mousefx::windows {
namespace {

Gdiplus::Color ResolveGraphNodeColor(bool highlighted) {
    return highlighted ? Gdiplus::Color(210, 255, 214, 138) : Gdiplus::Color(118, 143, 214, 255);
}

Gdiplus::Color ResolveLogicalLinkColor(const std::string& logicalNode) {
    if (logicalNode == "body") {
        return Gdiplus::Color(208, 124, 219, 170);
    }
    if (logicalNode == "head") {
        return Gdiplus::Color(214, 255, 196, 122);
    }
    if (logicalNode == "appendage") {
        return Gdiplus::Color(208, 255, 173, 126);
    }
    if (logicalNode == "overlay") {
        return Gdiplus::Color(220, 124, 192, 255);
    }
    if (logicalNode == "grounding") {
        return Gdiplus::Color(212, 220, 178, 112);
    }
    return Gdiplus::Color(192, 180, 180, 180);
}

float ResolveGraphNodeRadius(bool highlighted) {
    return highlighted ? 5.5f : 3.5f;
}

std::array<uint32_t, 5> CollectHighlightedNodeIndices(
    const Win32MouseCompanionRealRendererAssetNodeMatchGraphProfile& graphProfile) {
    return {
        graphProfile.bodyEntry.matchedNodeIndex,
        graphProfile.headEntry.matchedNodeIndex,
        graphProfile.appendageEntry.matchedNodeIndex,
        graphProfile.overlayEntry.matchedNodeIndex,
        graphProfile.groundingEntry.matchedNodeIndex,
    };
}

bool IsHighlightedNode(
    uint32_t nodeIndex,
    const Win32MouseCompanionRealRendererAssetNodeMatchGraphProfile& graphProfile) {
    if (!graphProfile.bodyEntry.resolved &&
        !graphProfile.headEntry.resolved &&
        !graphProfile.appendageEntry.resolved &&
        !graphProfile.overlayEntry.resolved &&
        !graphProfile.groundingEntry.resolved) {
        return false;
    }
    const auto highlighted = CollectHighlightedNodeIndices(graphProfile);
    return std::find(highlighted.begin(), highlighted.end(), nodeIndex) != highlighted.end();
}

void AppendLogicalLink(
    const std::string& logicalNode,
    const Gdiplus::PointF& anchor,
    const Win32MouseCompanionRealRendererAssetNodeMatchGraphEntry& graphEntry,
    const std::unordered_map<uint32_t, Gdiplus::PointF>& nodeCenters,
    std::vector<Win32MouseCompanionRealRendererSceneGraphLink>* links) {
    if (links == nullptr || !graphEntry.resolved) {
        return;
    }
    auto it = nodeCenters.find(graphEntry.matchedNodeIndex);
    if (it == nodeCenters.end()) {
        return;
    }
    links->push_back(Win32MouseCompanionRealRendererSceneGraphLink{
        logicalNode,
        anchor,
        it->second,
        ResolveLogicalLinkColor(logicalNode),
        182.0f,
    });
}

} // namespace

void BuildWin32MouseCompanionRealRendererModelSceneGraph(
    const Win32MouseCompanionRealRendererSceneRuntime& runtime,
    Win32MouseCompanionRealRendererScene& scene) {
    scene.modelSceneGraphVisible = false;
    scene.modelSceneGraphNodes.clear();
    scene.modelSceneGraphEdges.clear();
    scene.modelSceneGraphLinks.clear();

    if (runtime.assets == nullptr || !runtime.assets->modelNodeTreeLoaded) {
        return;
    }

    const auto& tree = runtime.assets->modelNodeTree;
    if (tree.nodes.empty()) {
        return;
    }

    const float graphWidth = std::max(scene.bodyRect.Width * 1.05f, 120.0f);
    const float graphHeight = std::max(scene.bodyRect.Height * 1.22f, 150.0f);
    const float graphLeft = scene.bodyRect.GetRight() + scene.bodyRect.Width * 0.12f;
    const float graphTop = scene.headRect.Y - scene.headRect.Height * 0.10f;
    scene.modelSceneGraphBounds = Gdiplus::RectF(graphLeft, graphTop, graphWidth, graphHeight);

    uint32_t maxDepth = 1;
    std::unordered_map<uint32_t, std::vector<uint32_t>> nodesByDepth;
    nodesByDepth.reserve(tree.nodes.size());
    for (const auto& node : tree.nodes) {
        const uint32_t depth = static_cast<uint32_t>(std::count(node.nodePath.begin(), node.nodePath.end(), '/'));
        nodesByDepth[depth].push_back(node.nodeIndex);
        maxDepth = std::max(maxDepth, depth);
    }

    std::unordered_map<uint32_t, Gdiplus::PointF> nodeCenters;
    nodeCenters.reserve(tree.nodes.size());

    for (uint32_t depth = 1; depth <= maxDepth; ++depth) {
        auto it = nodesByDepth.find(depth);
        if (it == nodesByDepth.end() || it->second.empty()) {
            continue;
        }
        const auto& indices = it->second;
        const float x = scene.modelSceneGraphBounds.X +
            scene.modelSceneGraphBounds.Width *
                (static_cast<float>(depth - 1) / std::max(1.0f, static_cast<float>(maxDepth - 1)));
        for (size_t i = 0; i < indices.size(); ++i) {
            const float y = scene.modelSceneGraphBounds.Y +
                scene.modelSceneGraphBounds.Height *
                    ((static_cast<float>(i) + 0.5f) / static_cast<float>(indices.size()));
            nodeCenters[indices[i]] = Gdiplus::PointF(x, y);
        }
    }

    scene.modelSceneGraphNodes.reserve(tree.nodes.size());
    for (const auto& node : tree.nodes) {
        auto centerIt = nodeCenters.find(node.nodeIndex);
        if (centerIt == nodeCenters.end()) {
            continue;
        }
        const bool highlighted = IsHighlightedNode(node.nodeIndex, runtime.assetNodeMatchGraphProfile);
        const float radius = ResolveGraphNodeRadius(highlighted);
        scene.modelSceneGraphNodes.push_back(
            Win32MouseCompanionRealRendererSceneGraphNode{
                node.nodeIndex,
                node.nodeName,
                node.nodePath,
                Gdiplus::RectF(
                    centerIt->second.X - radius,
                    centerIt->second.Y - radius,
                    radius * 2.0f,
                    radius * 2.0f),
                ResolveGraphNodeColor(highlighted),
                highlighted,
            });
    }

    for (const auto& node : tree.nodes) {
        auto startIt = nodeCenters.find(node.nodeIndex);
        if (startIt == nodeCenters.end()) {
            continue;
        }
        for (uint32_t childIndex : node.childIndices) {
            auto endIt = nodeCenters.find(childIndex);
            if (endIt == nodeCenters.end()) {
                continue;
            }
            const bool highlighted =
                IsHighlightedNode(node.nodeIndex, runtime.assetNodeMatchGraphProfile) ||
                IsHighlightedNode(childIndex, runtime.assetNodeMatchGraphProfile);
            scene.modelSceneGraphEdges.push_back(
                Win32MouseCompanionRealRendererSceneGraphEdge{
                    node.nodeIndex,
                    childIndex,
                    startIt->second,
                    endIt->second,
                    highlighted ? 168.0f : 92.0f,
                });
        }
    }

    scene.modelSceneGraphVisible = !scene.modelSceneGraphNodes.empty();
    if (!scene.modelSceneGraphVisible) {
        return;
    }

    scene.modelSceneGraphLinks.reserve(5);
    AppendLogicalLink(
        "body",
        scene.bodyAnchor,
        runtime.assetNodeMatchGraphProfile.bodyEntry,
        nodeCenters,
        &scene.modelSceneGraphLinks);
    AppendLogicalLink(
        "head",
        scene.headAnchor,
        runtime.assetNodeMatchGraphProfile.headEntry,
        nodeCenters,
        &scene.modelSceneGraphLinks);
    AppendLogicalLink(
        "appendage",
        scene.appendageAnchor,
        runtime.assetNodeMatchGraphProfile.appendageEntry,
        nodeCenters,
        &scene.modelSceneGraphLinks);
    AppendLogicalLink(
        "overlay",
        scene.overlayAnchor,
        runtime.assetNodeMatchGraphProfile.overlayEntry,
        nodeCenters,
        &scene.modelSceneGraphLinks);
    AppendLogicalLink(
        "grounding",
        scene.groundingAnchor,
        runtime.assetNodeMatchGraphProfile.groundingEntry,
        nodeCenters,
        &scene.modelSceneGraphLinks);
}

} // namespace mousefx::windows
