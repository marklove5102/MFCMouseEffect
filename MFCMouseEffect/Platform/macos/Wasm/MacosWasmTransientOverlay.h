#pragma once

#include <cstdint>
#include <string>

#include "MouseFx/Core/Config/EffectConfig.h"
#include "MouseFx/Core/Protocol/InputTypes.h"

namespace mousefx::platform::macos {

enum class WasmOverlayRenderResult : uint8_t {
    Rendered = 0,
    ThrottledByCapacity = 1,
    ThrottledByInterval = 2,
    Failed = 3,
};

struct WasmImageOverlayRequest final {
    ScreenPoint screenPt{};
    std::wstring assetPath{};
    uint32_t tintArgb = 0;
    float scale = 1.0f;
    float alpha = 1.0f;
    uint32_t lifeMs = 0;
    uint32_t delayMs = 0;
    float velocityX = 0.0f;
    float velocityY = 0.0f;
    float accelerationX = 0.0f;
    float accelerationY = 0.0f;
    float rotationRad = 0.0f;
    bool applyTint = false;
};

WasmOverlayRenderResult ShowWasmImageOverlay(const WasmImageOverlayRequest& request);

WasmOverlayRenderResult ShowWasmImagePulseOverlay(
    const ScreenPoint& screenPt,
    uint32_t tintArgb,
    float scale,
    float alpha,
    uint32_t lifeMs);

WasmOverlayRenderResult ShowWasmTextOverlay(
    const ScreenPoint& screenPt,
    const std::wstring& text,
    uint32_t argb,
    const TextConfig& textConfig);

void CloseAllWasmOverlays();

} // namespace mousefx::platform::macos
