#pragma once

#include "MouseFx/Core/Wasm/WasmPathCommandGeometry.h"
#include "MouseFx/Core/Wasm/WasmPluginAbi.h"

#include <gdiplus.h>
#include <vector>

namespace mousefx::wasm {

inline Gdiplus::LineJoin ResolvePathStrokeLineJoin(uint8_t lineJoin) {
    switch (lineJoin) {
    case kPathStrokeLineJoinMiter:
        return Gdiplus::LineJoinMiter;
    case kPathStrokeLineJoinBevel:
        return Gdiplus::LineJoinBevel;
    case kPathStrokeLineJoinRound:
    default:
        return Gdiplus::LineJoinRound;
    }
}

inline Gdiplus::LineCap ResolvePathStrokeLineCap(uint8_t lineCap) {
    switch (lineCap) {
    case kPathStrokeLineCapButt:
        return Gdiplus::LineCapFlat;
    case kPathStrokeLineCapSquare:
        return Gdiplus::LineCapSquare;
    case kPathStrokeLineCapRound:
    default:
        return Gdiplus::LineCapRound;
    }
}

inline Gdiplus::FillMode ResolvePathFillMode(uint8_t fillRule) {
    return (fillRule == kPathFillRuleEvenOdd) ? Gdiplus::FillModeAlternate : Gdiplus::FillModeWinding;
}

inline bool BuildPathGraphicsPath(
    const std::vector<ResolvedPathNode>& nodes,
    Gdiplus::FillMode fillMode,
    Gdiplus::GraphicsPath* outPath) {
    if (!outPath || nodes.empty()) {
        return false;
    }

    outPath->Reset();
    outPath->SetFillMode(fillMode);

    bool haveCurrent = false;
    bool haveSubpathStart = false;
    bool startedFigure = false;
    bool drewAnySegment = false;
    Gdiplus::PointF current{};
    Gdiplus::PointF subpathStart{};

    for (const ResolvedPathNode& node : nodes) {
        switch (node.opcode) {
        case kPathStrokeNodeOpMoveTo: {
            if (startedFigure) {
                outPath->StartFigure();
            } else {
                startedFigure = true;
            }
            current = Gdiplus::PointF(node.x1, node.y1);
            subpathStart = current;
            haveCurrent = true;
            haveSubpathStart = true;
            break;
        }
        case kPathStrokeNodeOpLineTo: {
            if (!haveCurrent) {
                return false;
            }
            const Gdiplus::PointF target(node.x1, node.y1);
            outPath->AddLine(current, target);
            current = target;
            drewAnySegment = true;
            break;
        }
        case kPathStrokeNodeOpQuadTo: {
            if (!haveCurrent) {
                return false;
            }
            const Gdiplus::PointF control(node.x1, node.y1);
            const Gdiplus::PointF target(node.x2, node.y2);
            const Gdiplus::PointF cubic1(
                current.X + (control.X - current.X) * (2.0f / 3.0f),
                current.Y + (control.Y - current.Y) * (2.0f / 3.0f));
            const Gdiplus::PointF cubic2(
                target.X + (control.X - target.X) * (2.0f / 3.0f),
                target.Y + (control.Y - target.Y) * (2.0f / 3.0f));
            outPath->AddBezier(current, cubic1, cubic2, target);
            current = target;
            drewAnySegment = true;
            break;
        }
        case kPathStrokeNodeOpCubicTo: {
            if (!haveCurrent) {
                return false;
            }
            const Gdiplus::PointF control1(node.x1, node.y1);
            const Gdiplus::PointF control2(node.x2, node.y2);
            const Gdiplus::PointF target(node.x3, node.y3);
            outPath->AddBezier(current, control1, control2, target);
            current = target;
            drewAnySegment = true;
            break;
        }
        case kPathStrokeNodeOpClose:
            if (!haveCurrent || !haveSubpathStart) {
                return false;
            }
            outPath->CloseFigure();
            current = subpathStart;
            break;
        default:
            return false;
        }
    }

    return drewAnySegment;
}

} // namespace mousefx::wasm
