#pragma once

#include <algorithm>
#include <cmath>

namespace mousefx::wasm {

constexpr float kGroupTransformIdentityScale = 1.0f;
constexpr float kGroupTransformMinScale = 0.05f;
constexpr float kGroupTransformMaxScale = 8.0f;

inline float ClampGroupTransformScale(float value) {
    if (!std::isfinite(value)) {
        return kGroupTransformIdentityScale;
    }
    return std::clamp(value, kGroupTransformMinScale, kGroupTransformMaxScale);
}

inline float AverageGroupTransformScale(float scaleX, float scaleY) {
    return (ClampGroupTransformScale(scaleX) + ClampGroupTransformScale(scaleY)) * 0.5f;
}

inline bool HasGeometryGroupTransform(bool useGroupLocalOrigin, float rotationRad, float scaleX, float scaleY) {
    if (!useGroupLocalOrigin) {
        return false;
    }
    return std::abs(rotationRad) > 0.001f ||
        std::abs(ClampGroupTransformScale(scaleX) - 1.0f) > 0.001f ||
        std::abs(ClampGroupTransformScale(scaleY) - 1.0f) > 0.001f;
}

struct GroupTransformVector final {
    float x = 0.0f;
    float y = 0.0f;
};

inline GroupTransformVector ApplyGroupTransformToPoint(
    float x,
    float y,
    float rotationRad,
    float scaleX,
    float scaleY,
    float pivotXPx = 0.0f,
    float pivotYPx = 0.0f) {
    const float resolvedScaleX = ClampGroupTransformScale(scaleX);
    const float resolvedScaleY = ClampGroupTransformScale(scaleY);
    const float cosTheta = std::cos(rotationRad);
    const float sinTheta = std::sin(rotationRad);
    const float localX = x - pivotXPx;
    const float localY = y - pivotYPx;
    const float scaledX = localX * resolvedScaleX;
    const float scaledY = localY * resolvedScaleY;
    return GroupTransformVector{
        scaledX * cosTheta - scaledY * sinTheta + pivotXPx,
        scaledX * sinTheta + scaledY * cosTheta + pivotYPx,
    };
}

inline GroupTransformVector ApplyGroupTransformToVector(
    float x,
    float y,
    float rotationRad,
    float scaleX,
    float scaleY) {
    return ApplyGroupTransformToPoint(x, y, rotationRad, scaleX, scaleY);
}

} // namespace mousefx::wasm
