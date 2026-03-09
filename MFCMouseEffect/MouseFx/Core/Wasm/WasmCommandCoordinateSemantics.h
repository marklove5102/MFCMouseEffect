#pragma once

namespace mousefx::wasm {

// WASM ABI uses screen-space semantics aligned with top-left origin:
// +X points right, +Y points down.
struct WasmCommandMotion final {
    float velocityX = 0.0f;
    float velocityY = 0.0f;
    float accelerationX = 0.0f;
    float accelerationY = 0.0f;
};

// Convert command motion into overlay-space semantics where +Y points up
// (for example Cocoa window coordinates on macOS overlays).
inline WasmCommandMotion ConvertMotionToOverlayYUp(const WasmCommandMotion& motion) {
    WasmCommandMotion converted = motion;
    converted.velocityY = -converted.velocityY;
    converted.accelerationY = -converted.accelerationY;
    return converted;
}

} // namespace mousefx::wasm
