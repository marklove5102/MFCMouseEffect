#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererPainter.h"

#include <algorithm>

namespace mousefx::windows {
namespace {

Gdiplus::Color WithAlpha(const Gdiplus::Color& color, float alpha) {
    return Gdiplus::Color(
        static_cast<BYTE>(std::clamp(alpha, 0.0f, 255.0f)),
        color.GetR(),
        color.GetG(),
        color.GetB());
}

void FillEllipse(
    Gdiplus::Graphics* graphics,
    const Gdiplus::RectF& rect,
    const Gdiplus::Color& fill,
    const Gdiplus::Color& stroke,
    float strokeWidth) {
    if (!graphics) {
        return;
    }
    Gdiplus::SolidBrush brush(fill);
    Gdiplus::Pen pen(stroke, strokeWidth);
    pen.SetAlignment(Gdiplus::PenAlignmentCenter);
    graphics->FillEllipse(&brush, rect);
    graphics->DrawEllipse(&pen, rect);
}

void FillRoundedRect(
    Gdiplus::Graphics* graphics,
    const Gdiplus::RectF& rect,
    const Gdiplus::Color& fill,
    const Gdiplus::Color& stroke,
    float strokeWidth) {
    if (!graphics) {
        return;
    }
    Gdiplus::GraphicsPath path;
    const float radius = std::min(rect.Width, rect.Height) * 0.42f;
    path.AddArc(rect.X, rect.Y, radius, radius, 180.0f, 90.0f);
    path.AddArc(rect.GetRight() - radius, rect.Y, radius, radius, 270.0f, 90.0f);
    path.AddArc(rect.GetRight() - radius, rect.GetBottom() - radius, radius, radius, 0.0f, 90.0f);
    path.AddArc(rect.X, rect.GetBottom() - radius, radius, radius, 90.0f, 90.0f);
    path.CloseFigure();
    Gdiplus::SolidBrush brush(fill);
    Gdiplus::Pen pen(stroke, strokeWidth);
    graphics->FillPath(&brush, &path);
    graphics->DrawPath(&pen, &path);
}

void FillEar(
    Gdiplus::Graphics* graphics,
    const std::array<Gdiplus::PointF, 4>& points,
    const Gdiplus::Color& fill,
    const Gdiplus::Color& inner,
    const Gdiplus::Color& stroke) {
    if (!graphics) {
        return;
    }
    Gdiplus::GraphicsPath path;
    path.AddClosedCurve(points.data(), static_cast<int>(points.size()));
    Gdiplus::SolidBrush fillBrush(fill);
    Gdiplus::Pen pen(stroke, 1.4f);
    graphics->FillPath(&fillBrush, &path);
    graphics->DrawPath(&pen, &path);

    Gdiplus::GraphicsPath innerPath;
    const Gdiplus::PointF innerPoints[4] = {
        Gdiplus::PointF(points[0].X, points[0].Y + 3.0f),
        Gdiplus::PointF((points[0].X + points[1].X) * 0.5f, (points[0].Y + points[1].Y) * 0.5f + 4.0f),
        Gdiplus::PointF(points[2].X, points[2].Y + 8.0f),
        Gdiplus::PointF((points[0].X + points[3].X) * 0.5f, (points[0].Y + points[3].Y) * 0.5f + 4.0f),
    };
    innerPath.AddClosedCurve(innerPoints, 4);
    Gdiplus::SolidBrush innerBrush(inner);
    graphics->FillPath(&innerBrush, &innerPath);
}

void DrawStar(
    Gdiplus::Graphics* graphics,
    const std::array<Gdiplus::PointF, 5>& points,
    const Gdiplus::Color& fill,
    const Gdiplus::Color& stroke) {
    if (!graphics) {
        return;
    }
    Gdiplus::GraphicsPath path;
    path.AddClosedCurve(points.data(), static_cast<int>(points.size()));
    Gdiplus::SolidBrush brush(fill);
    Gdiplus::Pen pen(stroke, 1.1f);
    graphics->FillPath(&brush, &path);
    graphics->DrawPath(&pen, &path);
}

void DrawActionOverlay(
    Gdiplus::Graphics* graphics,
    const Win32MouseCompanionRealRendererActionOverlay& overlay,
    const Gdiplus::Color& stroke) {
    if (!graphics) {
        return;
    }

    if (overlay.followTrailVisible) {
        for (size_t i = 0; i < overlay.followTrailRects.size(); ++i) {
            const float alpha = std::max(48.0f, overlay.followTrailBaseAlpha - static_cast<float>(i) * 34.0f);
            FillEllipse(
                graphics,
                overlay.followTrailRects[i],
                WithAlpha(overlay.accentColor, alpha),
                Gdiplus::Color(0, 0, 0, 0),
                0.0f);
        }
    }

    if (overlay.scrollArcVisible) {
        Gdiplus::Pen arcPen(WithAlpha(overlay.accentColor, overlay.scrollArcAlpha), overlay.scrollArcStrokeWidth);
        arcPen.SetStartCap(Gdiplus::LineCapRound);
        arcPen.SetEndCap(Gdiplus::LineCapRound);
        graphics->DrawArc(
            &arcPen,
            overlay.scrollArcRect,
            overlay.scrollArcStartDeg,
            overlay.scrollArcSweepDeg);
    }

    if (overlay.dragLineVisible) {
        Gdiplus::Pen dashPen(WithAlpha(overlay.accentColor, overlay.dragLineAlpha), overlay.dragLineStrokeWidth);
        dashPen.SetStartCap(Gdiplus::LineCapRound);
        dashPen.SetEndCap(Gdiplus::LineCapRound);
        const Gdiplus::REAL dashPattern[2] = {4.0f, 3.0f};
        dashPen.SetDashPattern(dashPattern, 2);
        graphics->DrawLine(&dashPen, overlay.dragLineStart, overlay.dragLineEnd);
    }

    if (overlay.clickRingVisible) {
        Gdiplus::Pen ringPen(WithAlpha(overlay.accentColor, overlay.clickRingAlpha), overlay.clickRingStrokeWidth);
        ringPen.SetAlignment(Gdiplus::PenAlignmentCenter);
        graphics->DrawEllipse(&ringPen, overlay.clickRingRect);
    }

    if (overlay.holdBandVisible) {
        FillRoundedRect(
            graphics,
            overlay.holdBandRect,
            WithAlpha(overlay.accentColor, overlay.holdBandAlpha),
            stroke,
            1.0f);
    }
}

} // namespace

void Win32MouseCompanionRealRendererPainter::Paint(
    const Win32MouseCompanionRealRendererScene& scene,
    Gdiplus::Graphics* graphics,
    int width,
    int height) const {
    if (!graphics || width <= 0 || height <= 0) {
        return;
    }

    const BYTE glowAlpha = static_cast<BYTE>(std::clamp(scene.glowAlpha, 0.0f, 255.0f));
    Gdiplus::SolidBrush glowBrush(Gdiplus::Color(
        glowAlpha,
        scene.glowColor.GetR(),
        scene.glowColor.GetG(),
        scene.glowColor.GetB()));
    Gdiplus::SolidBrush shadowBrush(scene.shadowFill);
    graphics->FillEllipse(&glowBrush, scene.glowRect);
    graphics->FillEllipse(&shadowBrush, scene.shadowRect);
    DrawActionOverlay(graphics, scene.actionOverlay, scene.bodyStroke);
    FillRoundedRect(graphics, scene.pedestalRect, scene.pedestalFill, scene.pedestalFill, 0.0f);
    FillEllipse(graphics, scene.tailRect, scene.tailFill, scene.bodyStroke, scene.tailStrokeWidth);
    FillEllipse(
        graphics,
        scene.tailRootCuffRect,
        WithAlpha(scene.bodyFillRear, 214.0f),
        Gdiplus::Color(0, 0, 0, 0),
        0.0f);
    FillEllipse(
        graphics,
        scene.tailTipRect,
        WithAlpha(scene.headFill, 236.0f),
        scene.bodyStroke,
        std::max(0.9f, scene.tailStrokeWidth - 0.2f));
    FillEllipse(
        graphics,
        scene.leftEarRootCuffRect,
        WithAlpha(scene.headFillRear, 214.0f),
        Gdiplus::Color(0, 0, 0, 0),
        0.0f);
    FillEllipse(
        graphics,
        scene.rightEarRootCuffRect,
        WithAlpha(scene.headFillRear, 214.0f),
        Gdiplus::Color(0, 0, 0, 0),
        0.0f);

    if (scene.facingSign < 0.0f) {
        FillEar(graphics, scene.rightEar, scene.earFillRear, scene.earInner, scene.bodyStroke);
        FillEar(graphics, scene.leftEar, scene.earFill, scene.earInner, scene.bodyStroke);
    } else {
        FillEar(graphics, scene.leftEar, scene.earFillRear, scene.earInner, scene.bodyStroke);
        FillEar(graphics, scene.rightEar, scene.earFill, scene.earInner, scene.bodyStroke);
    }

    FillRoundedRect(graphics, scene.leftLegRect, scene.bodyFillRear, scene.bodyStroke, scene.limbStrokeWidth);
    FillRoundedRect(graphics, scene.rightLegRect, scene.bodyFillRear, scene.bodyStroke, scene.limbStrokeWidth);
    FillEllipse(
        graphics,
        scene.leftLegSilhouetteBridgeRect,
        WithAlpha(scene.bodyFillRear, 144.0f),
        Gdiplus::Color(0, 0, 0, 0),
        0.0f);
    FillEllipse(
        graphics,
        scene.rightLegSilhouetteBridgeRect,
        WithAlpha(scene.bodyFillRear, 144.0f),
        Gdiplus::Color(0, 0, 0, 0),
        0.0f);
    FillEllipse(
        graphics,
        scene.leftLegRootCuffRect,
        WithAlpha(scene.bodyFillRear, 156.0f),
        Gdiplus::Color(0, 0, 0, 0),
        0.0f);
    FillEllipse(
        graphics,
        scene.rightLegRootCuffRect,
        WithAlpha(scene.bodyFillRear, 156.0f),
        Gdiplus::Color(0, 0, 0, 0),
        0.0f);
    FillEllipse(
        graphics,
        scene.leftLegPadRect,
        WithAlpha(scene.blushFill, 196.0f),
        Gdiplus::Color(0, 0, 0, 0),
        0.0f);
    FillEllipse(
        graphics,
        scene.rightLegPadRect,
        WithAlpha(scene.blushFill, 196.0f),
        Gdiplus::Color(0, 0, 0, 0),
        0.0f);

    const Gdiplus::GraphicsState saved = graphics->Save();
    const float cx = scene.bodyRect.X + scene.bodyRect.Width * 0.5f;
    const float cy = scene.bodyRect.Y + scene.bodyRect.Height * 0.5f;
    graphics->TranslateTransform(cx, cy);
    graphics->RotateTransform(scene.bodyTiltDeg);
    graphics->TranslateTransform(-cx, -cy);
    FillEllipse(graphics, scene.bodyRect, scene.bodyFill, scene.bodyStroke, scene.bodyStrokeWidth);
    FillEllipse(
        graphics,
        scene.leftShoulderPatchRect,
        WithAlpha(scene.bodyFillRear, 176.0f),
        Gdiplus::Color(0, 0, 0, 0),
        0.0f);
    FillEllipse(
        graphics,
        scene.rightShoulderPatchRect,
        WithAlpha(scene.bodyFillRear, 176.0f),
        Gdiplus::Color(0, 0, 0, 0),
        0.0f);
    FillRoundedRect(
        graphics,
        scene.neckBridgeRect,
        WithAlpha(scene.headFillRear, 210.0f),
        scene.bodyStroke,
        std::max(1.0f, scene.bodyStrokeWidth - 0.2f));
    FillEllipse(
        graphics,
        scene.chestRect,
        WithAlpha(scene.headFill, scene.chestFillAlpha),
        scene.bodyStroke,
        scene.chestStrokeWidth);
    FillEllipse(
        graphics,
        scene.leftHipPatchRect,
        WithAlpha(scene.bodyFillRear, 166.0f),
        Gdiplus::Color(0, 0, 0, 0),
        0.0f);
    FillEllipse(
        graphics,
        scene.rightHipPatchRect,
        WithAlpha(scene.bodyFillRear, 166.0f),
        Gdiplus::Color(0, 0, 0, 0),
        0.0f);
    FillEllipse(
        graphics,
        scene.bellyContourRect,
        WithAlpha(scene.headFillRear, 138.0f),
        Gdiplus::Color(0, 0, 0, 0),
        0.0f);
    FillEllipse(
        graphics,
        scene.sternumContourRect,
        WithAlpha(scene.headFillRear, 122.0f),
        Gdiplus::Color(0, 0, 0, 0),
        0.0f);
    FillEllipse(
        graphics,
        scene.upperTorsoContourRect,
        WithAlpha(scene.headFillRear, 132.0f),
        Gdiplus::Color(0, 0, 0, 0),
        0.0f);
    FillEllipse(
        graphics,
        scene.leftBackContourRect,
        WithAlpha(scene.bodyFillRear, 146.0f),
        Gdiplus::Color(0, 0, 0, 0),
        0.0f);
    FillEllipse(
        graphics,
        scene.rightBackContourRect,
        WithAlpha(scene.bodyFillRear, 146.0f),
        Gdiplus::Color(0, 0, 0, 0),
        0.0f);
    FillEllipse(
        graphics,
        scene.leftFlankContourRect,
        WithAlpha(scene.bodyFillRear, 150.0f),
        Gdiplus::Color(0, 0, 0, 0),
        0.0f);
    FillEllipse(
        graphics,
        scene.rightFlankContourRect,
        WithAlpha(scene.bodyFillRear, 150.0f),
        Gdiplus::Color(0, 0, 0, 0),
        0.0f);
    graphics->Restore(saved);

    FillRoundedRect(graphics, scene.leftHandRect, scene.headFillRear, scene.bodyStroke, scene.limbStrokeWidth);
    FillRoundedRect(graphics, scene.rightHandRect, scene.headFillRear, scene.bodyStroke, scene.limbStrokeWidth);
    FillEllipse(
        graphics,
        scene.leftHandSilhouetteBridgeRect,
        WithAlpha(scene.headFillRear, 150.0f),
        Gdiplus::Color(0, 0, 0, 0),
        0.0f);
    FillEllipse(
        graphics,
        scene.rightHandSilhouetteBridgeRect,
        WithAlpha(scene.headFillRear, 150.0f),
        Gdiplus::Color(0, 0, 0, 0),
        0.0f);
    FillEllipse(
        graphics,
        scene.leftHandRootCuffRect,
        WithAlpha(scene.headFillRear, 166.0f),
        Gdiplus::Color(0, 0, 0, 0),
        0.0f);
    FillEllipse(
        graphics,
        scene.rightHandRootCuffRect,
        WithAlpha(scene.headFillRear, 166.0f),
        Gdiplus::Color(0, 0, 0, 0),
        0.0f);
    FillEllipse(
        graphics,
        scene.leftHandPadRect,
        WithAlpha(scene.blushFill, 206.0f),
        Gdiplus::Color(0, 0, 0, 0),
        0.0f);
    FillEllipse(
        graphics,
        scene.rightHandPadRect,
        WithAlpha(scene.blushFill, 206.0f),
        Gdiplus::Color(0, 0, 0, 0),
        0.0f);
    FillEllipse(graphics, scene.headRect, scene.headFill, scene.bodyStroke, scene.headStrokeWidth);
    FillEllipse(
        graphics,
        scene.leftCheekContourRect,
        WithAlpha(scene.headFillRear, 168.0f),
        Gdiplus::Color(0, 0, 0, 0),
        0.0f);
    FillEllipse(
        graphics,
        scene.rightCheekContourRect,
        WithAlpha(scene.headFillRear, 168.0f),
        Gdiplus::Color(0, 0, 0, 0),
        0.0f);
    FillEllipse(
        graphics,
        scene.jawContourRect,
        WithAlpha(scene.headFillRear, 154.0f),
        Gdiplus::Color(0, 0, 0, 0),
        0.0f);
    FillEllipse(
        graphics,
        scene.muzzlePadRect,
        WithAlpha(scene.headFillRear, 188.0f),
        Gdiplus::Color(0, 0, 0, 0),
        0.0f);
    FillEllipse(
        graphics,
        scene.foreheadPadRect,
        WithAlpha(scene.headFillRear, 148.0f),
        Gdiplus::Color(0, 0, 0, 0),
        0.0f);
    FillEllipse(
        graphics,
        scene.leftTempleContourRect,
        WithAlpha(scene.headFillRear, 132.0f),
        Gdiplus::Color(0, 0, 0, 0),
        0.0f);
    FillEllipse(
        graphics,
        scene.rightTempleContourRect,
        WithAlpha(scene.headFillRear, 132.0f),
        Gdiplus::Color(0, 0, 0, 0),
        0.0f);
    FillEllipse(
        graphics,
        scene.leftUnderEyeContourRect,
        WithAlpha(scene.headFillRear, 126.0f),
        Gdiplus::Color(0, 0, 0, 0),
        0.0f);
    FillEllipse(
        graphics,
        scene.rightUnderEyeContourRect,
        WithAlpha(scene.headFillRear, 126.0f),
        Gdiplus::Color(0, 0, 0, 0),
        0.0f);
    FillEllipse(
        graphics,
        scene.noseBridgeRect,
        WithAlpha(scene.headFillRear, 118.0f),
        Gdiplus::Color(0, 0, 0, 0),
        0.0f);

    FillEllipse(graphics, scene.leftEyeRect, scene.eyeFill, scene.eyeFill, 0.0f);
    FillEllipse(graphics, scene.rightEyeRect, scene.eyeFill, scene.eyeFill, 0.0f);
    FillEllipse(graphics, scene.leftPupilRect, scene.mouthFill, scene.mouthFill, 0.0f);
    FillEllipse(graphics, scene.rightPupilRect, scene.mouthFill, scene.mouthFill, 0.0f);
    FillEllipse(
        graphics,
        scene.leftEyeHighlightRect,
        WithAlpha(Gdiplus::Color(255, 255, 255, 255), scene.eyeHighlightAlpha),
        Gdiplus::Color(0, 0, 0, 0),
        0.0f);
    FillEllipse(
        graphics,
        scene.rightEyeHighlightRect,
        WithAlpha(Gdiplus::Color(255, 255, 255, 255), scene.eyeHighlightAlpha),
        Gdiplus::Color(0, 0, 0, 0),
        0.0f);
    FillEllipse(graphics, scene.noseRect, scene.mouthFill, scene.mouthFill, 0.0f);
    FillEllipse(graphics, scene.leftBlushRect, scene.blushFill, scene.blushFill, 0.0f);
    FillEllipse(graphics, scene.rightBlushRect, scene.blushFill, scene.blushFill, 0.0f);

    {
        Gdiplus::Pen browPen(scene.eyeFill, 1.5f);
        browPen.SetStartCap(Gdiplus::LineCapRound);
        browPen.SetEndCap(Gdiplus::LineCapRound);
        graphics->DrawLine(&browPen, scene.leftBrowStart, scene.leftBrowEnd);
        graphics->DrawLine(&browPen, scene.rightBrowStart, scene.rightBrowEnd);
    }

    {
        Gdiplus::Pen whiskerPen(WithAlpha(scene.mouthFill, 210.0f), scene.whiskerStrokeWidth);
        whiskerPen.SetStartCap(Gdiplus::LineCapRound);
        whiskerPen.SetEndCap(Gdiplus::LineCapRound);
        for (size_t i = 0; i < scene.leftWhiskerStart.size(); ++i) {
            graphics->DrawLine(&whiskerPen, scene.leftWhiskerStart[i], scene.leftWhiskerEnd[i]);
            graphics->DrawLine(&whiskerPen, scene.rightWhiskerStart[i], scene.rightWhiskerEnd[i]);
        }
    }

    {
        Gdiplus::Pen mouthPen(scene.mouthFill, scene.mouthStrokeWidth);
        mouthPen.SetStartCap(Gdiplus::LineCapRound);
        mouthPen.SetEndCap(Gdiplus::LineCapRound);
        graphics->DrawArc(
            &mouthPen,
            scene.mouthRect,
            scene.mouthStartDeg,
            scene.mouthSweepDeg);
    }

    for (size_t i = 0; i < scene.laneBadgeRects.size(); ++i) {
        const auto fill = scene.laneReady[i] ? scene.badgeReadyFill : scene.badgePendingFill;
        FillRoundedRect(graphics, scene.laneBadgeRects[i], fill, scene.bodyStroke, 0.8f);
    }

    if (scene.poseBadgeVisible) {
        FillEllipse(graphics, scene.poseBadgeRect, scene.accentFill, scene.headFill, 1.0f);
    }
    if (scene.accessoryVisible) {
        DrawStar(graphics, scene.accessoryStar, scene.accessoryFill, scene.accessoryStroke);
    }
}

} // namespace mousefx::windows
