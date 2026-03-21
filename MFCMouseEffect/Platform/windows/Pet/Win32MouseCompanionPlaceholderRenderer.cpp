#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionPlaceholderRenderer.h"
#include "Platform/windows/Pet/Win32MouseCompanionPlaceholderScene.h"

#include <algorithm>
#include <cmath>

namespace mousefx::windows {
namespace {

Gdiplus::Color BlendColor(
    const Gdiplus::Color& base,
    const Gdiplus::Color& target,
    float t) {
    const float clamped = std::clamp(t, 0.0f, 1.0f);
    const auto lerp = [clamped](BYTE a, BYTE b) -> BYTE {
        return static_cast<BYTE>(std::lround(
            static_cast<double>(a) + (static_cast<double>(b) - static_cast<double>(a)) * clamped));
    };
    return Gdiplus::Color(
        lerp(base.GetA(), target.GetA()),
        lerp(base.GetR(), target.GetR()),
        lerp(base.GetG(), target.GetG()),
        lerp(base.GetB(), target.GetB()));
}

void FillRoundedEllipse(
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

void FillEar(
    Gdiplus::Graphics* graphics,
    const Gdiplus::PointF* points,
    int pointCount,
    const Gdiplus::Color& fill,
    const Gdiplus::Color& inner) {
    if (!graphics || !points || pointCount < 3) {
        return;
    }
    Gdiplus::GraphicsPath path;
    path.AddClosedCurve(points, pointCount);
    Gdiplus::SolidBrush fillBrush(fill);
    graphics->FillPath(&fillBrush, &path);

    Gdiplus::Matrix matrix;
    matrix.Translate(0.0f, 6.0f);
    path.Transform(&matrix);
    Gdiplus::SolidBrush innerBrush(inner);
    graphics->FillPath(&innerBrush, &path);
}

} // namespace

void Win32MouseCompanionPlaceholderRenderer::Render(
    const Win32MouseCompanionVisualState& state,
    Gdiplus::Graphics* graphics,
    int width,
    int height) const {
    if (!graphics || width <= 0 || height <= 0) {
        return;
    }

    graphics->SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    graphics->SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
    const Win32MouseCompanionPlaceholderScene scene =
        BuildWin32MouseCompanionPlaceholderScene(state, width, height);

    Gdiplus::SolidBrush glowBrush(Gdiplus::Color(
        static_cast<BYTE>(std::clamp(scene.glowAlpha, 0.0f, 255.0f)),
        scene.accent.GetR(),
        scene.accent.GetG(),
        scene.accent.GetB()));
    graphics->FillEllipse(
        &glowBrush,
        Gdiplus::RectF(
            scene.centerX - scene.bodyRect.Width * 1.2f + scene.bodyLeanPx * 0.18f,
            scene.bodyRect.Y - scene.bodyRect.Height * 0.05f,
            scene.bodyRect.Width * 2.4f,
            scene.bodyRect.Height * 1.9f));

    FillEar(graphics, scene.leftEar.data(), static_cast<int>(scene.leftEar.size()), scene.headFill, scene.earInner);
    FillEar(graphics, scene.rightEar.data(), static_cast<int>(scene.rightEar.size()), scene.headFill, scene.earInner);
    FillRoundedEllipse(graphics, scene.bodyRect, scene.bodyFill, scene.bodyStroke, 2.0f);
    FillRoundedEllipse(graphics, scene.headRect, scene.headFill, scene.bodyStroke, 2.0f);
    FillRoundedEllipse(graphics, scene.leftHandRect, scene.headFill, scene.bodyStroke, 1.6f);
    FillRoundedEllipse(graphics, scene.rightHandRect, scene.headFill, scene.bodyStroke, 1.6f);
    FillRoundedEllipse(graphics, scene.leftLegRect, scene.headFill, scene.bodyStroke, 1.4f);
    FillRoundedEllipse(graphics, scene.rightLegRect, scene.headFill, scene.bodyStroke, 1.4f);

    const float badgeSize = 7.0f;
    const float badgeTop = 10.0f;
    float badgeX = static_cast<float>(width) - 14.0f;
    if (scene.modelAssetAvailable) {
        Gdiplus::SolidBrush badge(Gdiplus::Color(220, 79, 193, 255));
        graphics->FillEllipse(&badge, Gdiplus::RectF(badgeX, badgeTop, badgeSize, badgeSize));
        badgeX -= 10.0f;
    }
    if (scene.actionLibraryAvailable) {
        Gdiplus::SolidBrush badge(Gdiplus::Color(220, 100, 208, 164));
        graphics->FillEllipse(&badge, Gdiplus::RectF(badgeX, badgeTop, badgeSize, badgeSize));
        badgeX -= 10.0f;
    }
    if (scene.poseBadgeVisible) {
        Gdiplus::SolidBrush badge(Gdiplus::Color(220, 255, 188, 96));
        graphics->FillEllipse(&badge, Gdiplus::RectF(badgeX, badgeTop, badgeSize, badgeSize));
    }

    if (scene.accessoryVisible) {
        Gdiplus::SolidBrush accessoryBrush(Gdiplus::Color(220, 255, 196, 120));
        graphics->FillEllipse(
            &accessoryBrush,
            Gdiplus::RectF(scene.centerX - 5.0f + scene.bodyLeanPx * 0.45f, scene.headRect.Y - 8.0f, 10.0f, 6.0f));
    }

    Gdiplus::SolidBrush eyeBrush(scene.eyeColor);
    Gdiplus::Pen mouthPen(scene.mouthColor, 1.6f);
    Gdiplus::SolidBrush blushBrush(scene.blushColor);
    graphics->FillEllipse(
        &eyeBrush,
        Gdiplus::RectF(scene.centerX - scene.headRect.Width * 0.20f + scene.bodyLeanPx * 0.45f, scene.headRect.Y + scene.headRect.Height * 0.42f, 5.0f, scene.eyeHeight));
    graphics->FillEllipse(
        &eyeBrush,
        Gdiplus::RectF(scene.centerX + scene.headRect.Width * 0.20f - 5.0f + scene.bodyLeanPx * 0.45f, scene.headRect.Y + scene.headRect.Height * 0.42f, 5.0f, scene.eyeHeight));
    graphics->DrawArc(
        &mouthPen,
        Gdiplus::RectF(scene.centerX - 6.0f + scene.bodyLeanPx * 0.45f, scene.headRect.Y + scene.headRect.Height * 0.56f, 12.0f, 8.0f),
        10.0f,
        160.0f);
    graphics->FillEllipse(
        &blushBrush,
        Gdiplus::RectF(scene.centerX - scene.headRect.Width * 0.32f + scene.bodyLeanPx * 0.45f, scene.headRect.Y + scene.headRect.Height * 0.58f, 9.0f, 5.0f));
    graphics->FillEllipse(
        &blushBrush,
        Gdiplus::RectF(scene.centerX + scene.headRect.Width * 0.32f - 9.0f + scene.bodyLeanPx * 0.45f, scene.headRect.Y + scene.headRect.Height * 0.58f, 9.0f, 5.0f));
}

} // namespace mousefx::windows
