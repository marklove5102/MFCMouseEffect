#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace mousefx::windows {

struct Win32MouseCompanionRealRendererGlbNodeEntry final {
    uint32_t nodeIndex{0};
    int32_t parentIndex{-1};
    std::string nodeName;
    std::string nodePath;
    std::vector<uint32_t> childIndices;
};

struct Win32MouseCompanionRealRendererGlbNodeTree final {
    std::string sourcePath;
    std::string rootNodeName;
    std::string sourceFormat{"phase1_placeholder"};
    std::vector<Win32MouseCompanionRealRendererGlbNodeEntry> nodes;
    bool loaded{false};
};

Win32MouseCompanionRealRendererGlbNodeTree LoadWin32MouseCompanionRealRendererGlbNodeTree(
    const std::string& modelPath);

} // namespace mousefx::windows
