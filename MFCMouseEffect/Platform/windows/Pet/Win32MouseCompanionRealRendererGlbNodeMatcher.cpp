#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererGlbNodeMatcher.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererNodeMatchNaming.h"

#include <algorithm>

namespace mousefx::windows {
namespace {

bool ContainsToken(const std::string& haystack, const std::string& needle) {
    return !needle.empty() && haystack.find(needle) != std::string::npos;
}

int ScoreLogicalNodeHint(
    const std::string& logicalNode,
    const std::string& normalizedName) {
    int score = 0;
    if (logicalNode == "body") {
        if (ContainsToken(normalizedName, "spine")) { score += 10; }
        if (ContainsToken(normalizedName, "hips")) { score += 8; }
        if (ContainsToken(normalizedName, "rootjoint")) { score += 7; }
        if (ContainsToken(normalizedName, "torso")) { score += 6; }
    } else if (logicalNode == "head") {
        if (ContainsToken(normalizedName, "head")) { score += 12; }
        if (ContainsToken(normalizedName, "neck")) { score += 8; }
        if (ContainsToken(normalizedName, "ear")) { score += 2; }
    } else if (logicalNode == "appendage") {
        if (ContainsToken(normalizedName, "arm")) { score += 10; }
        if (ContainsToken(normalizedName, "ear")) { score += 8; }
        if (ContainsToken(normalizedName, "leg")) { score += 6; }
        if (ContainsToken(normalizedName, "tail")) { score += 5; }
    } else if (logicalNode == "overlay") {
        if (ContainsToken(normalizedName, "object")) { score += 8; }
        if (ContainsToken(normalizedName, "effect")) { score += 8; }
        if (ContainsToken(normalizedName, "overlay")) { score += 8; }
    } else if (logicalNode == "grounding") {
        if (ContainsToken(normalizedName, "root")) { score += 10; }
        if (ContainsToken(normalizedName, "leg")) { score += 7; }
        if (ContainsToken(normalizedName, "spine")) { score += 4; }
    }
    return score;
}

int ScoreTokenList(
    const std::string& normalizedName,
    const std::vector<std::string>& tokens,
    int pointsPerToken,
    const char** reason) {
    int score = 0;
    for (const std::string& token : tokens) {
        if (!ContainsToken(normalizedName, token)) {
            continue;
        }
        score += pointsPerToken;
        if (reason != nullptr && *reason == nullptr) {
            *reason = "token";
        }
    }
    return score;
}

float ResolveConfidenceFromScore(int score) {
    if (score <= 0) {
        return 0.0f;
    }
    return std::clamp(0.35f + static_cast<float>(score) * 0.025f, 0.0f, 0.98f);
}

uint32_t ResolveNodeDepth(const std::string& nodePath) {
    if (nodePath.empty()) {
        return 0;
    }
    uint32_t depth = 0;
    for (char ch : nodePath) {
        if (ch == '/') {
            ++depth;
        }
    }
    return depth;
}

std::string ResolveSemanticTag(
    const std::string& logicalNode,
    const std::string& normalizedName,
    const std::string& normalizedPath) {
    const std::string combined = normalizedName + "|" + normalizedPath;
    if (ContainsToken(combined, "head")) {
        return "head";
    }
    if (ContainsToken(combined, "neck")) {
        return "neck";
    }
    if (ContainsToken(combined, "ear")) {
        return "ear";
    }
    if (ContainsToken(combined, "arm")) {
        return "arm";
    }
    if (ContainsToken(combined, "leg")) {
        return "leg";
    }
    if (ContainsToken(combined, "spine")) {
        return "spine";
    }
    if (ContainsToken(combined, "root")) {
        return "root";
    }
    if (ContainsToken(combined, "object") || ContainsToken(combined, "overlay")) {
        return "overlay";
    }
    return logicalNode.empty() ? "unknown" : logicalNode;
}

} // namespace

Win32MouseCompanionRealRendererGlbNodeMatchResult ResolveWin32MouseCompanionRealRendererGlbNodeMatch(
    const Win32MouseCompanionRealRendererGlbNodeTree& tree,
    const std::string& logicalNode,
    const std::string& selectorKey,
    const std::string& candidateNodeName,
    const std::string& alias,
    const std::string& label,
    const std::string& tokenSeed) {
    Win32MouseCompanionRealRendererGlbNodeMatchResult result{};
    if (!tree.loaded || tree.nodes.empty()) {
        return result;
    }

    const std::vector<std::string> selectorTokens =
        TokenizeWin32MouseCompanionNodeMatchText(selectorKey);
    const std::vector<std::string> candidateTokens =
        TokenizeWin32MouseCompanionNodeMatchText(candidateNodeName);
    const std::vector<std::string> aliasTokens =
        TokenizeWin32MouseCompanionNodeMatchText(alias);
    const std::vector<std::string> labelTokens =
        TokenizeWin32MouseCompanionNodeMatchText(label);
    const std::vector<std::string> seedTokens =
        TokenizeWin32MouseCompanionNodeMatchText(tokenSeed);

    const Win32MouseCompanionRealRendererGlbNodeEntry* bestNode = nullptr;
    int bestScore = 0;
    std::string bestBasis;

    for (const auto& node : tree.nodes) {
        const std::string normalizedName =
            NormalizeWin32MouseCompanionNodeMatchToken(node.nodeName);
        const std::string normalizedPath =
            NormalizeWin32MouseCompanionNodeMatchToken(node.nodePath);
        int score = ScoreLogicalNodeHint(logicalNode, normalizedName);
        const char* reason = nullptr;

        score += ScoreTokenList(normalizedName, candidateTokens, 4, &reason);
        score += ScoreTokenList(normalizedName, aliasTokens, 3, &reason);
        score += ScoreTokenList(normalizedName, labelTokens, 2, &reason);
        score += ScoreTokenList(normalizedName, selectorTokens, 2, &reason);
        score += ScoreTokenList(normalizedPath, seedTokens, 2, &reason);

        if (score <= bestScore) {
            continue;
        }

        bestNode = &node;
        bestScore = score;
        bestBasis = reason == nullptr ? "logical" : reason;
    }

    if (bestNode == nullptr || bestScore <= 0) {
        return result;
    }

    result.nodeIndex = bestNode->nodeIndex;
    result.nodeName = bestNode->nodeName;
    result.nodePath = bestNode->nodePath;
    if (bestNode->parentIndex >= 0 &&
        static_cast<size_t>(bestNode->parentIndex) < tree.nodes.size()) {
        result.parentNodeName = tree.nodes[static_cast<size_t>(bestNode->parentIndex)].nodeName;
    }
    const std::string normalizedName =
        NormalizeWin32MouseCompanionNodeMatchToken(bestNode->nodeName);
    const std::string normalizedPath =
        NormalizeWin32MouseCompanionNodeMatchToken(bestNode->nodePath);
    result.semanticTag = ResolveSemanticTag(logicalNode, normalizedName, normalizedPath);
    result.matchBasis = bestBasis.empty() ? "logical" : bestBasis;
    result.nodeDepth = ResolveNodeDepth(bestNode->nodePath);
    result.confidence = ResolveConfidenceFromScore(bestScore);
    result.matched = true;
    return result;
}

} // namespace mousefx::windows
