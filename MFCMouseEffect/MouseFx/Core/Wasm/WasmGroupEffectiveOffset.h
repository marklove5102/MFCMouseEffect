#pragma once

#include "MouseFx/Core/Wasm/WasmGroupLocalOriginRuntime.h"
#include "MouseFx/Core/Wasm/WasmGroupTransformRuntime.h"

namespace mousefx::wasm {

struct GroupEffectiveOffset final {
    float offsetXPx = 0.0f;
    float offsetYPx = 0.0f;
};

struct GroupEffectiveTransform final {
    float offsetXPx = 0.0f;
    float offsetYPx = 0.0f;
    float rotationRad = 0.0f;
    float uniformScale = 1.0f;
    float pivotXPx = 0.0f;
    float pivotYPx = 0.0f;
    float scaleX = 1.0f;
    float scaleY = 1.0f;
};

inline GroupEffectiveOffset ResolveEffectiveGroupOffset(
    const std::wstring& activeManifestPath,
    uint32_t groupId,
    bool useGroupLocalOrigin) {
    const GroupTransformState transformState = ResolveGroupTransform(activeManifestPath, groupId);
    GroupEffectiveOffset effective{
        transformState.offsetXPx,
        transformState.offsetYPx,
    };
    if (!useGroupLocalOrigin) {
        return effective;
    }

    const GroupLocalOriginState localOriginState = ResolveGroupLocalOrigin(activeManifestPath, groupId);
    effective.offsetXPx += localOriginState.originXPx;
    effective.offsetYPx += localOriginState.originYPx;
    return effective;
}

inline GroupEffectiveTransform ResolveEffectiveGroupTransform(
    const std::wstring& activeManifestPath,
    uint32_t groupId,
    bool useGroupLocalOrigin) {
    const GroupTransformState transformState = ResolveGroupTransform(activeManifestPath, groupId);
    GroupEffectiveTransform effective{
        transformState.offsetXPx,
        transformState.offsetYPx,
        transformState.rotationRad,
        transformState.uniformScale,
        transformState.pivotXPx,
        transformState.pivotYPx,
        transformState.scaleX,
        transformState.scaleY,
    };
    if (!useGroupLocalOrigin) {
        effective.rotationRad = 0.0f;
        effective.uniformScale = 1.0f;
        effective.pivotXPx = 0.0f;
        effective.pivotYPx = 0.0f;
        effective.scaleX = 1.0f;
        effective.scaleY = 1.0f;
        return effective;
    }

    const GroupLocalOriginState localOriginState = ResolveGroupLocalOrigin(activeManifestPath, groupId);
    effective.offsetXPx += localOriginState.originXPx;
    effective.offsetYPx += localOriginState.originYPx;
    return effective;
}

} // namespace mousefx::wasm
