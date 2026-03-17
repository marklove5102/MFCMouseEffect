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

void PetDispatchFeature::OnButtonDown(AppController& controller, const ScreenPoint& pt, int button) const {
    controller.DispatchPetButtonDown(pt, button);
}

void PetDispatchFeature::OnButtonUp(AppController& controller, const ScreenPoint& pt, int button) const {
    controller.DispatchPetButtonUp(pt, button);
}

} // namespace mousefx
