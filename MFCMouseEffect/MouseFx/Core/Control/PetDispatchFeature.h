#pragma once

#include "MouseFx/Core/Protocol/InputTypes.h"

namespace mousefx {

class AppController;

// Dispatch adapter for mouse-companion action routing.
class PetDispatchFeature final {
public:
    void OnClick(AppController& controller, const ClickEvent& ev) const;
    void OnMouseMove(AppController& controller, const ScreenPoint& pt) const;
    void OnScroll(AppController& controller, const ScreenPoint& pt, int delta) const;
    void OnButtonDown(AppController& controller, const ScreenPoint& pt, int button) const;
    void OnButtonUp(AppController& controller, const ScreenPoint& pt, int button) const;
    void OnHoverStart(AppController& controller, const ScreenPoint& pt) const;
    void OnHoverEnd(AppController& controller, const ScreenPoint& pt) const;
    void OnHoldStart(AppController& controller, const ScreenPoint& pt, int button, uint32_t holdMs) const;
    void OnHoldUpdate(AppController& controller, const ScreenPoint& pt, uint32_t holdMs) const;
    void OnHoldEnd(AppController& controller, const ScreenPoint& pt) const;
};

} // namespace mousefx
