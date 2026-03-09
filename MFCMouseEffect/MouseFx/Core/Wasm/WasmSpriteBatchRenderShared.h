#pragma once

#include "MouseFx/Core/Wasm/WasmSpriteBatchCommandConfig.h"
#include "MouseFx/Renderers/RenderUtils.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <memory>
#include <vector>

namespace mousefx::wasm::sprite_batch_render_shared {

inline float Clamp01(float value) {
    return std::clamp(value, 0.0f, 1.0f);
}

class BitmapCache final {
public:
    Gdiplus::Image* EnsureImageLoaded(const std::wstring& path) {
        if (path.empty()) {
            return nullptr;
        }

        for (BitmapCacheEntry& entry : cache_) {
            if (entry.path != path) {
                continue;
            }
            if (!entry.attempted || !entry.image) {
                return nullptr;
            }
            return entry.image.get();
        }

        BitmapCacheEntry entry{};
        entry.path = path;
        entry.attempted = true;
        std::unique_ptr<Gdiplus::Image> image(Gdiplus::Image::FromFile(path.c_str(), FALSE));
        if (image && image->GetLastStatus() == Gdiplus::Ok) {
            entry.image = std::move(image);
        }
        cache_.push_back(std::move(entry));
        return cache_.back().image.get();
    }

private:
    struct BitmapCacheEntry final {
        std::wstring path{};
        std::unique_ptr<Gdiplus::Image> image{};
        bool attempted = false;
    };

    std::vector<BitmapCacheEntry> cache_{};
};

inline void ApplyTintColorMatrix(
    uint32_t tintArgb,
    bool applyTint,
    float alphaScale,
    float outMatrix[5][5]) {
    for (int row = 0; row < 5; ++row) {
        for (int col = 0; col < 5; ++col) {
            outMatrix[row][col] = (row == col) ? 1.0f : 0.0f;
        }
    }

    const float effectiveAlpha = Clamp01(alphaScale);
    outMatrix[3][3] = effectiveAlpha;
    if (!applyTint) {
        return;
    }

    const float tintA = static_cast<float>((tintArgb >> 24) & 0xFFu) / 255.0f;
    const float tintR = static_cast<float>((tintArgb >> 16) & 0xFFu) / 255.0f;
    const float tintG = static_cast<float>((tintArgb >> 8) & 0xFFu) / 255.0f;
    const float tintB = static_cast<float>(tintArgb & 0xFFu) / 255.0f;
    outMatrix[0][0] = tintR;
    outMatrix[1][1] = tintG;
    outMatrix[2][2] = tintB;
    outMatrix[3][3] = Clamp01(effectiveAlpha * tintA);
}

inline void DrawFallbackSprite(
    Gdiplus::Graphics& g,
    const ResolvedSpriteBatchItem& item,
    float drawX,
    float drawY,
    float widthPx,
    float heightPx,
    float alphaScale,
    bool screenBlend) {
    const float sizePx = std::max(widthPx, heightPx);
    const float radius = sizePx * 0.5f;
    const Gdiplus::Color base = render_utils::ToGdiPlus({item.tintArgb});
    const BYTE coreAlpha = render_utils::ClampByte(
        static_cast<int>(static_cast<float>(base.GetA()) * alphaScale));
    if (coreAlpha == 0u) {
        return;
    }

    const float glowRadius = radius * (screenBlend ? 1.95f : 1.65f);
    Gdiplus::GraphicsPath path;
    path.AddEllipse(drawX + radius - glowRadius, drawY + radius - glowRadius, glowRadius * 2.0f, glowRadius * 2.0f);
    Gdiplus::PathGradientBrush glowBrush(&path);
    const Gdiplus::Color centerColor(
        render_utils::ClampByte(static_cast<int>(coreAlpha * (screenBlend ? 0.68f : 0.46f))),
        base.GetR(),
        base.GetG(),
        base.GetB());
    const Gdiplus::Color surroundColor(0u, base.GetR(), base.GetG(), base.GetB());
    INT count = 1;
    glowBrush.SetCenterColor(centerColor);
    glowBrush.SetSurroundColors(const_cast<Gdiplus::Color*>(&surroundColor), &count);
    g.FillPath(&glowBrush, &path);

    const Gdiplus::GraphicsState state = g.Save();
    g.TranslateTransform(drawX + widthPx * 0.5f, drawY + heightPx * 0.5f);
    if (std::abs(item.rotationRad) > 0.001f) {
        constexpr float kRadToDeg = 57.29577951308232f;
        g.RotateTransform(item.rotationRad * kRadToDeg);
    }
    g.TranslateTransform(-widthPx * 0.5f, -heightPx * 0.5f);
    Gdiplus::SolidBrush coreBrush(
        Gdiplus::Color(coreAlpha, base.GetR(), base.GetG(), base.GetB()));
    g.FillEllipse(&coreBrush, 0.0f, 0.0f, widthPx, heightPx);
    g.Restore(state);
}

inline void DrawImageSprite(
    Gdiplus::Graphics& g,
    const ResolvedSpriteBatchItem& item,
    Gdiplus::Image* image,
    float drawX,
    float drawY,
    float widthPx,
    float heightPx,
    float alphaScale) {
    if (image == nullptr) {
        return;
    }

    const float imageW = static_cast<float>(image->GetWidth());
    const float imageH = static_cast<float>(image->GetHeight());
    if (imageW <= 0.0f || imageH <= 0.0f) {
        return;
    }
    const float srcU0 = Clamp01(std::min(item.srcU0, item.srcU1));
    const float srcV0 = Clamp01(std::min(item.srcV0, item.srcV1));
    const float srcU1 = Clamp01(std::max(item.srcU0, item.srcU1));
    const float srcV1 = Clamp01(std::max(item.srcV0, item.srcV1));
    const float srcX = srcU0 * imageW;
    const float srcY = srcV0 * imageH;
    const float srcW = std::max(1.0f, (srcU1 - srcU0) * imageW);
    const float srcH = std::max(1.0f, (srcV1 - srcV0) * imageH);

    float matrixData[5][5] = {};
    ApplyTintColorMatrix(item.tintArgb, item.applyTint, alphaScale, matrixData);
    Gdiplus::ColorMatrix matrix;
    std::memcpy(&matrix, matrixData, sizeof(matrix));
    Gdiplus::ImageAttributes attrs;
    attrs.SetColorMatrix(&matrix, Gdiplus::ColorMatrixFlagsDefault, Gdiplus::ColorAdjustTypeBitmap);

    const Gdiplus::GraphicsState state = g.Save();
    g.TranslateTransform(drawX + widthPx * 0.5f, drawY + heightPx * 0.5f);
    if (std::abs(item.rotationRad) > 0.001f) {
        constexpr float kRadToDeg = 57.29577951308232f;
        g.RotateTransform(item.rotationRad * kRadToDeg);
    }
    g.TranslateTransform(-widthPx * 0.5f, -heightPx * 0.5f);
    g.DrawImage(
        image,
        Gdiplus::RectF(0.0f, 0.0f, widthPx, heightPx),
        srcX,
        srcY,
        srcW,
        srcH,
        Gdiplus::UnitPixel,
        &attrs);
    g.Restore(state);
}

inline void DrawResolvedSprite(
    Gdiplus::Graphics& g,
    const ResolvedSpriteBatchItem& item,
    float x,
    float y,
    float alphaScale,
    bool screenBlend,
    BitmapCache* cache) {
    const float widthPx = std::max(2.0f, item.widthPx);
    const float heightPx = std::max(2.0f, item.heightPx);
    const float drawX = x - widthPx * 0.5f;
    const float drawY = y - heightPx * 0.5f;
    Gdiplus::Image* image = cache ? cache->EnsureImageLoaded(item.assetPath) : nullptr;
    if (image != nullptr) {
        DrawImageSprite(g, item, image, drawX, drawY, widthPx, heightPx, alphaScale);
    } else {
        DrawFallbackSprite(g, item, drawX, drawY, widthPx, heightPx, alphaScale, screenBlend);
    }
}

} // namespace mousefx::wasm::sprite_batch_render_shared
