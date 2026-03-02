#pragma once

#include "MouseFx/Core/Control/IDispatchMessageCodec.h"

#include <cstdint>

namespace mousefx {

// POSIX/macOS dispatch message codec. Reuses the same WM_MFX_* ids as Windows.
class MacosDispatchMessageCodec final : public IDispatchMessageCodec {
public:
    static constexpr uint32_t kTimerMessageId = 0x0113u; // WM_TIMER-compatible id.

    DispatchMessage Decode(
        uintptr_t sourceHandle,
        uint32_t msg,
        uintptr_t wParam,
        intptr_t lParam) const override;

    intptr_t DefaultResult(
        uintptr_t sourceHandle,
        uint32_t msg,
        uintptr_t wParam,
        intptr_t lParam) const override;

    static intptr_t PackPointPayload(int32_t x, int32_t y);
    static int32_t UnpackPointX(intptr_t payload);
    static int32_t UnpackPointY(intptr_t payload);
};

} // namespace mousefx
