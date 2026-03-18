#include "pch.h"

#include "MouseFx/Core/Control/PetDispatchFeature.h"

#include "MouseFx/Core/Control/AppController.h"

namespace mousefx {

void PetDispatchFeature::OnClick(AppController& controller, const ClickEvent& ev) const {
    controller.DispatchPetClick(ev);
}

void PetDispatchFeature::OnMouseMove(AppController& controller, const ScreenPoint& pt) const {
    controller.DispatchPetMove(pt);
}

void PetDispatchFeature::OnScroll(AppController& controller, const ScreenPoint& pt, int delta) const {
    controller.DispatchPetScroll(pt, delta);
}

void PetDispatchFeature::OnButtonDown(AppController& controller, const ScreenPoint& pt, int button) const {
    controller.DispatchPetButtonDown(pt, button);
}

void PetDispatchFeature::OnButtonUp(AppController& controller, const ScreenPoint& pt, int button) const {
    controller.DispatchPetButtonUp(pt, button);
}

void PetDispatchFeature::OnHoverStart(AppController& controller, const ScreenPoint& pt) const {
    controller.DispatchPetHoverStart(pt);
}

void PetDispatchFeature::OnHoverEnd(AppController& controller, const ScreenPoint& pt) const {
    controller.DispatchPetHoverEnd(pt);
}

void PetDispatchFeature::OnHoldStart(
    AppController& controller,
    const ScreenPoint& pt,
    int button,
    uint32_t holdMs) const {
    controller.DispatchPetHoldStart(pt, button, holdMs);
}

void PetDispatchFeature::OnHoldUpdate(
    AppController& controller,
    const ScreenPoint& pt,
    uint32_t holdMs) const {
    controller.DispatchPetHoldUpdate(pt, holdMs);
}

void PetDispatchFeature::OnHoldEnd(AppController& controller, const ScreenPoint& pt) const {
    controller.DispatchPetHoldEnd(pt);
}

} // namespace mousefx
