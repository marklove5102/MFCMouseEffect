#pragma once

#include "MouseFx/Core/Overlay/OverlayCoordSpace.h"
#include "MouseFx/Core/Wasm/WasmPluginAbi.h"
#include "MouseFx/Interfaces/RenderSemantics.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <string>

namespace mousefx::wasm {

inline RenderBlendMode ResolveRenderBlendMode(uint8_t rawBlendMode) {
    switch (rawBlendMode) {
    case kCommandBlendModeScreen:
        return RenderBlendMode::Screen;
    case kCommandBlendModeAdd:
        return RenderBlendMode::Add;
    case kCommandBlendModeNormal:
    default:
        return RenderBlendMode::Normal;
    }
}

inline void ResolveRenderClipRectToOverlaySpace(RenderClipRect* clipRect) {
    if (!clipRect) {
        return;
    }
    if (!(clipRect->widthPx > 0.0f) || !(clipRect->heightPx > 0.0f)) {
        clipRect->leftPx = 0.0f;
        clipRect->topPx = 0.0f;
        clipRect->widthPx = 0.0f;
        clipRect->heightPx = 0.0f;
        return;
    }

    const ScreenPoint screenOrigin{
        static_cast<int32_t>(std::lround(static_cast<double>(clipRect->leftPx))),
        static_cast<int32_t>(std::lround(static_cast<double>(clipRect->topPx))),
    };
    const ScreenPoint screenFar{
        static_cast<int32_t>(std::lround(static_cast<double>(clipRect->leftPx + clipRect->widthPx))),
        static_cast<int32_t>(std::lround(static_cast<double>(clipRect->topPx + clipRect->heightPx))),
    };
    const ScreenPoint overlayOrigin = ScreenToOverlayPoint(screenOrigin);
    const ScreenPoint overlayFar = ScreenToOverlayPoint(screenFar);

    const int32_t minX = std::min(overlayOrigin.x, overlayFar.x);
    const int32_t maxX = std::max(overlayOrigin.x, overlayFar.x);
    const int32_t minY = std::min(overlayOrigin.y, overlayFar.y);
    const int32_t maxY = std::max(overlayOrigin.y, overlayFar.y);
    clipRect->leftPx = static_cast<float>(minX);
    clipRect->topPx = static_cast<float>(minY);
    clipRect->widthPx = static_cast<float>(std::max<int32_t>(0, maxX - minX));
    clipRect->heightPx = static_cast<float>(std::max<int32_t>(0, maxY - minY));
}

inline bool TryResolveOptionalCommandRenderSemanticsTail(
    const uint8_t* raw,
    size_t sizeBytes,
    size_t payloadBytes,
    bool legacyScreenBlend,
    RenderSemantics* outSemantics,
    std::string* outError,
    const char* commandName) {
    if (!outSemantics) {
        if (outError) {
            *outError = std::string(commandName ? commandName : "command") + " semantics output is null";
        }
        return false;
    }

    outSemantics->blendMode = legacyScreenBlend ? RenderBlendMode::Screen : RenderBlendMode::Normal;
    outSemantics->sortKey = 0;
    outSemantics->groupId = 0u;
    outSemantics->clipRect = {};

    if (sizeBytes <= payloadBytes) {
        return true;
    }

    const size_t tailBytes = sizeBytes - payloadBytes;
    if (tailBytes < sizeof(CommandRenderSemanticsTailV1)) {
        if (outError) {
            *outError = std::string(commandName ? commandName : "command") + " render semantics tail truncated";
        }
        return false;
    }

    CommandRenderSemanticsTailV1 tail{};
    std::memcpy(&tail, raw + payloadBytes, sizeof(tail));
    outSemantics->blendMode = ResolveRenderBlendMode(tail.blendMode);
    outSemantics->sortKey = tail.sortKey;
    outSemantics->groupId = tail.groupId;
    return true;
}

inline bool TryResolveOptionalCommandRenderSemanticsAndClipRectTail(
    const uint8_t* raw,
    size_t sizeBytes,
    size_t payloadBytes,
    bool legacyScreenBlend,
    RenderSemantics* outSemantics,
    std::string* outError,
    const char* commandName) {
    if (!outSemantics) {
        if (outError) {
            *outError = std::string(commandName ? commandName : "command") + " semantics output is null";
        }
        return false;
    }

    outSemantics->blendMode = legacyScreenBlend ? RenderBlendMode::Screen : RenderBlendMode::Normal;
    outSemantics->sortKey = 0;
    outSemantics->groupId = 0u;
    outSemantics->clipRect = {};

    if (sizeBytes <= payloadBytes) {
        return true;
    }

    const size_t tailBytes = sizeBytes - payloadBytes;
    const size_t semanticsTailBytes = sizeof(CommandRenderSemanticsTailV1);
    const size_t clipTailBytes = sizeof(CommandClipRectTailV1);
    if (tailBytes != semanticsTailBytes && tailBytes != semanticsTailBytes + clipTailBytes) {
        if (outError) {
            if (tailBytes < semanticsTailBytes) {
                *outError = std::string(commandName ? commandName : "command") + " render semantics tail truncated";
            } else if (tailBytes < semanticsTailBytes + clipTailBytes) {
                *outError = std::string(commandName ? commandName : "command") + " clip_rect tail truncated";
            } else {
                *outError = std::string(commandName ? commandName : "command") + " tail size is unsupported";
            }
        }
        return false;
    }

    CommandRenderSemanticsTailV1 semanticsTail{};
    std::memcpy(&semanticsTail, raw + payloadBytes, sizeof(semanticsTail));
    outSemantics->blendMode = ResolveRenderBlendMode(semanticsTail.blendMode);
    outSemantics->sortKey = semanticsTail.sortKey;
    outSemantics->groupId = semanticsTail.groupId;

    if (tailBytes == semanticsTailBytes + clipTailBytes) {
        CommandClipRectTailV1 clipTail{};
        std::memcpy(&clipTail, raw + payloadBytes + semanticsTailBytes, sizeof(clipTail));
        outSemantics->clipRect.leftPx = clipTail.leftPx;
        outSemantics->clipRect.topPx = clipTail.topPx;
        outSemantics->clipRect.widthPx = clipTail.widthPx;
        outSemantics->clipRect.heightPx = clipTail.heightPx;
        ResolveRenderClipRectToOverlaySpace(&outSemantics->clipRect);
    }

    return true;
}

} // namespace mousefx::wasm
