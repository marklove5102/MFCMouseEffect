#pragma once

#include "MouseFx/Core/Overlay/OverlayCoordSpace.h"
#include "MouseFx/Core/Wasm/WasmPluginAbi.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

namespace mousefx::wasm {

constexpr uint16_t kMaxSpawnPathNodes = 128u;

struct ResolvedPathNode final {
    uint8_t opcode = kPathStrokeNodeOpMoveTo;
    float x1 = 0.0f;
    float y1 = 0.0f;
    float x2 = 0.0f;
    float y2 = 0.0f;
    float x3 = 0.0f;
    float y3 = 0.0f;
};

struct ResolvedPathGeometry final {
    ScreenPoint centerScreenPt{};
    int frameLeftPx = 0;
    int frameTopPx = 0;
    int squareSizePx = 64;
    std::vector<ResolvedPathNode> localNodes{};
};

inline float ClampPathCommandFloat(float value, float fallback, float minValue, float maxValue) {
    if (!std::isfinite(value)) {
        return fallback;
    }
    return std::clamp(value, minValue, maxValue);
}

inline uint32_t ResolvePathCommandLifeMs(uint32_t lifeMs, uint32_t fallbackLifeMs) {
    const uint32_t base = (lifeMs > 0u) ? lifeMs : fallbackLifeMs;
    return std::clamp<uint32_t>(base, 40u, 8000u);
}

inline uint32_t ResolvePathCommandDelayMs(uint32_t delayMs) {
    return std::min<uint32_t>(delayMs, 60000u);
}

inline uint32_t ScalePathCommandArgb(uint32_t argb, float alphaScale) {
    const float clampedAlpha = std::clamp(alphaScale, 0.0f, 1.0f);
    const uint32_t baseAlpha = (argb >> 24) & 0xFFu;
    const uint32_t scaledAlpha = static_cast<uint32_t>(
        std::lround(static_cast<double>(baseAlpha) * static_cast<double>(clampedAlpha)));
    return (argb & 0x00FFFFFFu) | ((scaledAlpha & 0xFFu) << 24);
}

inline uint32_t ResolvePathCommandGlowColor(uint32_t baseArgb, uint32_t glowArgb) {
    if (glowArgb != 0u) {
        return glowArgb;
    }
    return (baseArgb & 0x00FFFFFFu) | 0x66000000u;
}

inline ScreenPoint ClampPathPoint(float x, float y) {
    const double clampedX = std::clamp(static_cast<double>(x), -32768.0, 32768.0);
    const double clampedY = std::clamp(static_cast<double>(y), -32768.0, 32768.0);
    return ScreenPoint{
        static_cast<int32_t>(std::lround(clampedX)),
        static_cast<int32_t>(std::lround(clampedY)),
    };
}

inline bool TryResolvePathCommandGeometry(
    const uint8_t* raw,
    size_t sizeBytes,
    size_t nodesOffset,
    uint16_t nodeCount,
    int paddingPx,
    const char* commandName,
    ResolvedPathGeometry* outResolved,
    std::string* outError) {
    if (!raw || !outResolved) {
        if (outError) {
            *outError = std::string(commandName ? commandName : "path_command") + " command buffer is null";
        }
        return false;
    }
    if (nodeCount == 0u) {
        if (outError) {
            *outError = std::string(commandName ? commandName : "path_command") + " requires at least 1 node";
        }
        return false;
    }
    if (nodeCount > kMaxSpawnPathNodes) {
        if (outError) {
            *outError = std::string(commandName ? commandName : "path_command") + " node_count exceeds limit";
        }
        return false;
    }

    const size_t nodesBytes = static_cast<size_t>(nodeCount) * sizeof(PathStrokeNodeV1);
    if (nodesOffset + nodesBytes > sizeBytes) {
        if (outError) {
            *outError = std::string(commandName ? commandName : "path_command") + " node payload truncated";
        }
        return false;
    }

    float screenMinX = 0.0f;
    float screenMaxX = 0.0f;
    float screenMinY = 0.0f;
    float screenMaxY = 0.0f;
    int overlayMinX = 0;
    int overlayMaxX = 0;
    int overlayMinY = 0;
    int overlayMaxY = 0;
    bool havePoint = false;
    bool haveCurrent = false;
    bool haveSubpathStart = false;
    int drawableSegmentCount = 0;
    ScreenPoint currentScreenPoint{};
    ScreenPoint subpathStartScreenPoint{};

    struct ResolvedNodeScreen final {
        ResolvedPathNode node{};
    };
    std::vector<ResolvedNodeScreen> resolvedNodes{};
    resolvedNodes.reserve(nodeCount);

    auto includePoint = [&](const ScreenPoint& screenPoint) {
        const ScreenPoint overlayPoint = ScreenToOverlayPoint(screenPoint);
        const float screenX = static_cast<float>(screenPoint.x);
        const float screenY = static_cast<float>(screenPoint.y);
        if (!havePoint) {
            screenMinX = screenMaxX = screenX;
            screenMinY = screenMaxY = screenY;
            overlayMinX = overlayMaxX = overlayPoint.x;
            overlayMinY = overlayMaxY = overlayPoint.y;
            havePoint = true;
            return;
        }
        screenMinX = std::min(screenMinX, screenX);
        screenMaxX = std::max(screenMaxX, screenX);
        screenMinY = std::min(screenMinY, screenY);
        screenMaxY = std::max(screenMaxY, screenY);
        overlayMinX = std::min(overlayMinX, overlayPoint.x);
        overlayMaxX = std::max(overlayMaxX, overlayPoint.x);
        overlayMinY = std::min(overlayMinY, overlayPoint.y);
        overlayMaxY = std::max(overlayMaxY, overlayPoint.y);
    };

    for (uint16_t index = 0; index < nodeCount; ++index) {
        PathStrokeNodeV1 node{};
        std::memcpy(
            &node,
            raw + nodesOffset + static_cast<size_t>(index) * sizeof(PathStrokeNodeV1),
            sizeof(node));

        ResolvedNodeScreen resolvedNode{};
        resolvedNode.node.opcode = node.opcode;
        switch (node.opcode) {
        case kPathStrokeNodeOpMoveTo: {
            const ScreenPoint target = ClampPathPoint(node.x1, node.y1);
            includePoint(target);
            currentScreenPoint = target;
            subpathStartScreenPoint = target;
            haveCurrent = true;
            haveSubpathStart = true;
            resolvedNode.node.x1 = static_cast<float>(target.x);
            resolvedNode.node.y1 = static_cast<float>(target.y);
            break;
        }
        case kPathStrokeNodeOpLineTo: {
            if (!haveCurrent) {
                if (outError) {
                    *outError = std::string(commandName ? commandName : "path_command") +
                        " line_to requires a current point";
                }
                return false;
            }
            const ScreenPoint target = ClampPathPoint(node.x1, node.y1);
            includePoint(target);
            currentScreenPoint = target;
            resolvedNode.node.x1 = static_cast<float>(target.x);
            resolvedNode.node.y1 = static_cast<float>(target.y);
            drawableSegmentCount += 1;
            break;
        }
        case kPathStrokeNodeOpQuadTo: {
            if (!haveCurrent) {
                if (outError) {
                    *outError = std::string(commandName ? commandName : "path_command") +
                        " quad_to requires a current point";
                }
                return false;
            }
            const ScreenPoint control = ClampPathPoint(node.x1, node.y1);
            const ScreenPoint target = ClampPathPoint(node.x2, node.y2);
            includePoint(control);
            includePoint(target);
            currentScreenPoint = target;
            resolvedNode.node.x1 = static_cast<float>(control.x);
            resolvedNode.node.y1 = static_cast<float>(control.y);
            resolvedNode.node.x2 = static_cast<float>(target.x);
            resolvedNode.node.y2 = static_cast<float>(target.y);
            drawableSegmentCount += 1;
            break;
        }
        case kPathStrokeNodeOpCubicTo: {
            if (!haveCurrent) {
                if (outError) {
                    *outError = std::string(commandName ? commandName : "path_command") +
                        " cubic_to requires a current point";
                }
                return false;
            }
            const ScreenPoint control1 = ClampPathPoint(node.x1, node.y1);
            const ScreenPoint control2 = ClampPathPoint(node.x2, node.y2);
            const ScreenPoint target = ClampPathPoint(node.x3, node.y3);
            includePoint(control1);
            includePoint(control2);
            includePoint(target);
            currentScreenPoint = target;
            resolvedNode.node.x1 = static_cast<float>(control1.x);
            resolvedNode.node.y1 = static_cast<float>(control1.y);
            resolvedNode.node.x2 = static_cast<float>(control2.x);
            resolvedNode.node.y2 = static_cast<float>(control2.y);
            resolvedNode.node.x3 = static_cast<float>(target.x);
            resolvedNode.node.y3 = static_cast<float>(target.y);
            drawableSegmentCount += 1;
            break;
        }
        case kPathStrokeNodeOpClose:
            if (!haveCurrent || !haveSubpathStart) {
                if (outError) {
                    *outError = std::string(commandName ? commandName : "path_command") +
                        " close requires an open subpath";
                }
                return false;
            }
            if (currentScreenPoint.x != subpathStartScreenPoint.x ||
                currentScreenPoint.y != subpathStartScreenPoint.y) {
                drawableSegmentCount += 1;
            }
            currentScreenPoint = subpathStartScreenPoint;
            break;
        default:
            if (outError) {
                *outError = std::string(commandName ? commandName : "path_command") +
                    " node opcode is unsupported";
            }
            return false;
        }

        resolvedNodes.push_back(std::move(resolvedNode));
    }

    if (!havePoint) {
        if (outError) {
            *outError = std::string(commandName ? commandName : "path_command") + " resolved empty path";
        }
        return false;
    }
    if (drawableSegmentCount <= 0) {
        if (outError) {
            *outError = std::string(commandName ? commandName : "path_command") +
                " requires at least 1 drawable segment";
        }
        return false;
    }

    ResolvedPathGeometry resolved{};
    const int clampedPaddingPx = std::max(0, paddingPx);
    const int widthPx = std::max(1, overlayMaxX - overlayMinX);
    const int heightPx = std::max(1, overlayMaxY - overlayMinY);
    const int sidePx = std::clamp(std::max(widthPx, heightPx) + clampedPaddingPx * 2, 32, 1024);
    const int offsetX = (sidePx - widthPx) / 2;
    const int offsetY = (sidePx - heightPx) / 2;
    resolved.frameLeftPx = overlayMinX - offsetX;
    resolved.frameTopPx = overlayMinY - offsetY;
    resolved.squareSizePx = sidePx;

    resolved.localNodes.reserve(resolvedNodes.size());
    for (const ResolvedNodeScreen& node : resolvedNodes) {
        ResolvedPathNode localNode = node.node;
        auto toLocal = [&](float screenX, float screenY, float* outX, float* outY) {
            const ScreenPoint overlayPoint = ScreenToOverlayPoint(ScreenPoint{
                static_cast<int32_t>(std::lround(screenX)),
                static_cast<int32_t>(std::lround(screenY)),
            });
            *outX = static_cast<float>(overlayPoint.x - resolved.frameLeftPx);
            *outY = static_cast<float>(overlayPoint.y - resolved.frameTopPx);
        };

        switch (localNode.opcode) {
        case kPathStrokeNodeOpMoveTo:
        case kPathStrokeNodeOpLineTo:
            toLocal(localNode.x1, localNode.y1, &localNode.x1, &localNode.y1);
            break;
        case kPathStrokeNodeOpQuadTo:
            toLocal(localNode.x1, localNode.y1, &localNode.x1, &localNode.y1);
            toLocal(localNode.x2, localNode.y2, &localNode.x2, &localNode.y2);
            break;
        case kPathStrokeNodeOpCubicTo:
            toLocal(localNode.x1, localNode.y1, &localNode.x1, &localNode.y1);
            toLocal(localNode.x2, localNode.y2, &localNode.x2, &localNode.y2);
            toLocal(localNode.x3, localNode.y3, &localNode.x3, &localNode.y3);
            break;
        case kPathStrokeNodeOpClose:
        default:
            break;
        }
        resolved.localNodes.push_back(localNode);
    }

    resolved.centerScreenPt.x = static_cast<int32_t>(std::lround((screenMinX + screenMaxX) * 0.5f));
    resolved.centerScreenPt.y = static_cast<int32_t>(std::lround((screenMinY + screenMaxY) * 0.5f));
    *outResolved = std::move(resolved);
    return true;
}

} // namespace mousefx::wasm
