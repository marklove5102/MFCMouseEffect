#pragma once

#include "MouseFx/Interfaces/RenderSemantics.h"

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>

namespace mousefx::wasm {

struct GroupClipRectState final {
    mousefx::RenderClipRect clipRect{};
    uint8_t maskShapeKind = kGroupClipMaskShapeRect;
    float cornerRadiusPx = 0.0f;
};

inline std::wstring BuildGroupClipRectKey(const std::wstring& activeManifestPath, uint32_t groupId) {
    if (activeManifestPath.empty() || groupId == 0u) {
        return {};
    }
    return activeManifestPath + L"#group-clip#" + std::to_wstring(groupId);
}

inline std::mutex& GroupClipRectMutex() {
    static std::mutex mutex;
    return mutex;
}

inline std::unordered_map<std::wstring, GroupClipRectState>& GroupClipRectEntries() {
    static std::unordered_map<std::wstring, GroupClipRectState> entries;
    return entries;
}

inline bool HasRenderClipRect(const mousefx::RenderClipRect& clipRect) {
    return clipRect.widthPx > 0.0f && clipRect.heightPx > 0.0f;
}

inline mousefx::RenderClipRect IntersectRenderClipRects(
    const mousefx::RenderClipRect& baseClipRect,
    const mousefx::RenderClipRect& groupClipRect) {
    if (!HasRenderClipRect(baseClipRect)) {
        return groupClipRect;
    }
    if (!HasRenderClipRect(groupClipRect)) {
        return baseClipRect;
    }

    const float leftPx = std::max(baseClipRect.leftPx, groupClipRect.leftPx);
    const float topPx = std::max(baseClipRect.topPx, groupClipRect.topPx);
    const float rightPx = std::min(
        baseClipRect.leftPx + baseClipRect.widthPx,
        groupClipRect.leftPx + groupClipRect.widthPx);
    const float bottomPx = std::min(
        baseClipRect.topPx + baseClipRect.heightPx,
        groupClipRect.topPx + groupClipRect.heightPx);
    if (rightPx <= leftPx || bottomPx <= topPx) {
        return {};
    }
    return mousefx::RenderClipRect{
        leftPx,
        topPx,
        rightPx - leftPx,
        bottomPx - topPx,
    };
}

inline GroupClipRectState ResolveGroupClipRectState(
    const std::wstring& activeManifestPath,
    uint32_t groupId) {
    if (activeManifestPath.empty() || groupId == 0u) {
        return {};
    }

    std::lock_guard<std::mutex> lock(GroupClipRectMutex());
    const auto& entries = GroupClipRectEntries();
    const auto it = entries.find(BuildGroupClipRectKey(activeManifestPath, groupId));
    if (it == entries.end()) {
        return {};
    }
    return it->second;
}

inline mousefx::RenderClipRect ResolveGroupClipRect(
    const std::wstring& activeManifestPath,
    uint32_t groupId) {
    return ResolveGroupClipRectState(activeManifestPath, groupId).clipRect;
}

inline void UpsertGroupClipRect(
    const std::wstring& activeManifestPath,
    uint32_t groupId,
    const mousefx::RenderClipRect& clipRect,
    uint8_t maskShapeKind = kGroupClipMaskShapeRect,
    float cornerRadiusPx = 0.0f) {
    const std::wstring key = BuildGroupClipRectKey(activeManifestPath, groupId);
    if (key.empty()) {
        return;
    }

    std::lock_guard<std::mutex> lock(GroupClipRectMutex());
    if (HasRenderClipRect(clipRect)) {
        GroupClipRectEntries()[key] = GroupClipRectState{
            clipRect,
            maskShapeKind <= kGroupClipMaskShapeEllipse ? maskShapeKind : kGroupClipMaskShapeRect,
            std::max(0.0f, cornerRadiusPx),
        };
    } else {
        GroupClipRectEntries().erase(key);
    }
}

inline void RemoveGroupClipRect(
    const std::wstring& activeManifestPath,
    uint32_t groupId) {
    const std::wstring key = BuildGroupClipRectKey(activeManifestPath, groupId);
    if (key.empty()) {
        return;
    }

    std::lock_guard<std::mutex> lock(GroupClipRectMutex());
    GroupClipRectEntries().erase(key);
}

inline void ResetGroupClipRectsForManifest(const std::wstring& activeManifestPath) {
    if (activeManifestPath.empty()) {
        return;
    }

    std::lock_guard<std::mutex> lock(GroupClipRectMutex());
    auto& entries = GroupClipRectEntries();
    for (auto it = entries.begin(); it != entries.end();) {
        if (it->first.rfind(activeManifestPath + L"#group-clip#", 0) == 0) {
            it = entries.erase(it);
        } else {
            ++it;
        }
    }
}

inline void ResetAllGroupClipRects() {
    std::lock_guard<std::mutex> lock(GroupClipRectMutex());
    GroupClipRectEntries().clear();
}

} // namespace mousefx::wasm
