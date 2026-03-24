#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererGraphSignal.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererAssetNodeMatchGraphProfile.h"

#include <algorithm>

namespace mousefx::windows {
namespace {

float ResolveBasisSignal(const std::string& matchBasis) {
    if (matchBasis == "logical") {
        return 0.26f;
    }
    if (matchBasis == "token") {
        return 0.34f;
    }
    if (matchBasis == "alias") {
        return 0.28f;
    }
    return 0.18f;
}

float ResolveDepthSignal(uint32_t depth) {
    return std::min(0.20f + static_cast<float>(depth) * 0.05f, 0.42f);
}

} // namespace

float ResolveWin32MouseCompanionRealRendererGraphEntrySignal(
    const Win32MouseCompanionRealRendererAssetNodeMatchGraphEntry& entry) {
    float signal = entry.graphConfidence * 0.58f;
    if (!entry.graphLocator.empty() && entry.graphLocator.front() == '/') {
        signal += 0.16f;
    }
    if (!entry.graphParentNodeLabel.empty()) {
        signal += 0.08f;
    }
    signal += ResolveBasisSignal(entry.matchBasis);
    signal += ResolveDepthSignal(entry.graphDepth) * 0.35f;
    return std::min(signal, 1.0f);
}

float ResolveWin32MouseCompanionRealRendererGraphSemanticSignal(
    const Win32MouseCompanionRealRendererAssetNodeMatchGraphEntry& entry,
    const std::string& expectedSemantic) {
    if (expectedSemantic.empty()) {
        return 0.0f;
    }
    if (entry.semanticTag == expectedSemantic) {
        return 1.0f;
    }
    if ((expectedSemantic == "appendage") &&
        (entry.semanticTag == "ear" || entry.semanticTag == "arm" || entry.semanticTag == "leg")) {
        return 0.82f;
    }
    if ((expectedSemantic == "grounding") &&
        (entry.semanticTag == "leg" || entry.semanticTag == "root")) {
        return 0.84f;
    }
    if ((expectedSemantic == "body") &&
        (entry.semanticTag == "spine" || entry.semanticTag == "root")) {
        return 0.86f;
    }
    if ((expectedSemantic == "head") &&
        (entry.semanticTag == "head" || entry.semanticTag == "neck")) {
        return 0.88f;
    }
    if ((expectedSemantic == "overlay") && entry.semanticTag == "overlay") {
        return 0.90f;
    }
    return 0.0f;
}

} // namespace mousefx::windows
