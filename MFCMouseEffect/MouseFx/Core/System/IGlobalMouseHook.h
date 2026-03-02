#pragma once

#include <cstdint>

#include "MouseFx/Core/Protocol/InputTypes.h"

namespace mousefx {

class IDispatchMessageHost;

// Cross-platform input hook abstraction.
class IGlobalMouseHook {
public:
    virtual ~IGlobalMouseHook() = default;

    virtual bool Start(IDispatchMessageHost* dispatchHost) = 0;
    virtual void Stop() = 0;
    virtual uint32_t LastError() const = 0;
    virtual bool ConsumeLatestMove(ScreenPoint& outPt) = 0;
    virtual void SetKeyboardCaptureExclusive(bool enabled) = 0;
};

} // namespace mousefx
