#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererModelProxyActionPresenceProjector.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererScene.h"

#include <algorithm>

namespace mousefx::windows {
namespace {

float ResolveHeadBias(const Win32MouseCompanionRealRendererScene& scene) {
    float bias = 0.0f;
    if (scene.modelProxyActionLayer.clickShellVisible) {
        bias += 1.0f;
    }
    if (scene.modelProxyActionLayer.scrollShellVisible) {
        bias += 0.64f;
    }
    if (scene.modelProxyActionLayer.followShellVisible) {
        bias += 0.28f;
    }
    return std::clamp(bias, 0.0f, 1.8f);
}

float ResolveAppendageBias(const Win32MouseCompanionRealRendererScene& scene) {
    float bias = 0.0f;
    if (scene.modelProxyActionLayer.holdShellVisible) {
        bias += 1.0f;
    }
    if (scene.modelProxyActionLayer.dragShellVisible) {
        bias += 0.92f;
    }
    if (scene.modelProxyActionLayer.followShellVisible) {
        bias += 0.72f;
    }
    if (scene.modelProxyActionLayer.scrollShellVisible) {
        bias += 0.22f;
    }
    return std::clamp(bias, 0.0f, 2.2f);
}

float ResolveAdornmentBias(const Win32MouseCompanionRealRendererScene& scene) {
    float bias = 0.0f;
    if (scene.modelProxyActionLayer.followShellVisible) {
        bias += 1.0f;
    }
    if (scene.modelProxyActionLayer.scrollShellVisible) {
        bias += 0.84f;
    }
    if (scene.modelProxyActionLayer.clickShellVisible) {
        bias += 0.40f;
    }
    return std::clamp(bias, 0.0f, 1.9f);
}

float Blend(float current, float target, float mix) {
    return current + (target - current) * mix;
}

} // namespace

void ApplyWin32MouseCompanionRealRendererModelProxyActionPresenceProjector(
    Win32MouseCompanionRealRendererScene& scene) {
    if (!scene.modelProxyVisible) {
        return;
    }

    const float dominance = std::clamp(scene.proxyDominance, 0.0f, 1.0f);
    const float headBias = ResolveHeadBias(scene);
    const float appendageBias = ResolveAppendageBias(scene);
    const float adornmentBias = ResolveAdornmentBias(scene);

    if (headBias > 0.0f) {
        const float mix = std::clamp(0.36f + dominance * 0.28f, 0.0f, 0.76f);
        scene.modelProxyFrameLayer.fillAlphaScale = Blend(
            scene.modelProxyFrameLayer.fillAlphaScale,
            1.04f + headBias * 0.10f,
            mix);
        scene.modelProxyFrameLayer.strokeAlphaScale = Blend(
            scene.modelProxyFrameLayer.strokeAlphaScale,
            1.06f + headBias * 0.12f,
            mix);
        scene.modelProxyContourLayer.strokeAlphaScale = Blend(
            scene.modelProxyContourLayer.strokeAlphaScale,
            1.06f + headBias * 0.14f,
            mix);
        scene.modelProxyDetailLayer.eyeAlphaScale = Blend(
            scene.modelProxyDetailLayer.eyeAlphaScale,
            1.08f + headBias * 0.16f,
            mix);
        scene.modelProxyDetailLayer.highlightAlphaScale = Blend(
            scene.modelProxyDetailLayer.highlightAlphaScale,
            1.10f + headBias * 0.18f,
            mix);
        scene.modelProxyDetailLayer.mouthAlphaScale = Blend(
            scene.modelProxyDetailLayer.mouthAlphaScale,
            1.04f + headBias * 0.12f,
            mix);
        scene.previewHeadAlphaScale *= std::max(0.14f, 0.32f - headBias * 0.06f);
        scene.previewDetailAlphaScale *= std::max(0.10f, 0.26f - headBias * 0.05f);
    }

    if (appendageBias > 0.0f) {
        const float mix = std::clamp(0.34f + dominance * 0.34f, 0.0f, 0.82f);
        scene.modelProxyFrameLayer.appendageAlphaScale = Blend(
            scene.modelProxyFrameLayer.appendageAlphaScale,
            1.08f + appendageBias * 0.14f,
            mix);
        scene.modelProxyAppendageLayer.rearAlphaScale = Blend(
            scene.modelProxyAppendageLayer.rearAlphaScale,
            1.08f + appendageBias * 0.18f,
            mix);
        scene.modelProxyAppendageLayer.accentAlphaScale = Blend(
            scene.modelProxyAppendageLayer.accentAlphaScale,
            1.10f + appendageBias * 0.20f,
            mix);
        scene.modelProxyAppendageLayer.strokeAlphaScale = Blend(
            scene.modelProxyAppendageLayer.strokeAlphaScale,
            1.06f + appendageBias * 0.16f,
            mix);
        scene.previewAppendageAlphaScale *= std::max(0.08f, 0.22f - appendageBias * 0.04f);
        scene.previewTailAlphaScale *= std::max(0.08f, 0.20f - appendageBias * 0.04f);
        scene.previewHandAlphaScale *= std::max(0.08f, 0.18f - appendageBias * 0.04f);
        scene.previewLegAlphaScale *= std::max(0.08f, 0.19f - appendageBias * 0.04f);
    }

    if (adornmentBias > 0.0f) {
        const float mix = std::clamp(0.30f + dominance * 0.30f, 0.0f, 0.72f);
        scene.modelProxyAdornmentLayer.laneAlphaScale = Blend(
            scene.modelProxyAdornmentLayer.laneAlphaScale,
            1.08f + adornmentBias * 0.16f,
            mix);
        scene.modelProxyAdornmentLayer.poseBadgeAlphaScale = Blend(
            scene.modelProxyAdornmentLayer.poseBadgeAlphaScale,
            1.08f + adornmentBias * 0.14f,
            mix);
        scene.modelProxyAdornmentLayer.accessoryAlphaScale = Blend(
            scene.modelProxyAdornmentLayer.accessoryAlphaScale,
            1.10f + adornmentBias * 0.18f,
            mix);
        scene.modelProxyAdornmentLayer.accessoryStrokeScale = Blend(
            scene.modelProxyAdornmentLayer.accessoryStrokeScale,
            1.04f + adornmentBias * 0.10f,
            mix);
        scene.previewAdornmentAlphaScale *= std::max(0.10f, 0.24f - adornmentBias * 0.05f);
    }
}

} // namespace mousefx::windows
