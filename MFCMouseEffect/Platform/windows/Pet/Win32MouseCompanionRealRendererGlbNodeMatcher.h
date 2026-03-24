#pragma once

#include <cstdint>
#include <string>

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererGlbNodeTree.h"

namespace mousefx::windows {

struct Win32MouseCompanionRealRendererGlbNodeMatchResult final {
    uint32_t nodeIndex{0};
    std::string nodeName;
    std::string nodePath;
    std::string matchBasis;
    float confidence{0.0f};
    bool matched{false};
};

Win32MouseCompanionRealRendererGlbNodeMatchResult ResolveWin32MouseCompanionRealRendererGlbNodeMatch(
    const Win32MouseCompanionRealRendererGlbNodeTree& tree,
    const std::string& logicalNode,
    const std::string& selectorKey,
    const std::string& candidateNodeName,
    const std::string& alias,
    const std::string& label,
    const std::string& tokenSeed);

} // namespace mousefx::windows
