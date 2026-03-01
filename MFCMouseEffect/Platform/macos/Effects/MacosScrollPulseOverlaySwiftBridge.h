#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void* mfx_macos_scroll_pulse_overlay_create_v1(
    double frameX,
    double frameY,
    double frameSize,
    double bodyX,
    double bodyY,
    double bodyWidth,
    double bodyHeight,
    int overlayX,
    int overlayY,
    int horizontal,
    int delta,
    int helixMode,
    int twinkleMode,
    unsigned int fillArgb,
    unsigned int strokeArgb,
    double baseOpacity,
    double durationSec);

#ifdef __cplusplus
}
#endif

