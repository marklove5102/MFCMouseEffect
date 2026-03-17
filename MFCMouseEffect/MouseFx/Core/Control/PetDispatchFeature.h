#pragma once

#include "MouseFx/Core/Protocol/InputTypes.h"

namespace mousefx {

class AppController;

// Dispatch adapter for mouse-companion action routing.
class PetDispatchFeature final {
public:
    void OnClick(AppController& controller, const ClickEvent& ev) const;
    void OnMouseMove(AppController& controller, const ScreenPoint& pt) const;
    void OnButtonDown(AppController& controller, const ScreenPoint& pt, int button) const;
    void OnButtonUp(AppController& controller, const ScreenPoint& pt, int button) const;
};

} // namespace mousefx
