#pragma once

#include "MouseFx/Core/Effects/ClickEffectCompute.h"
#include "MouseFx/Core/Effects/HoldEffectCompute.h"
#include "MouseFx/Core/Effects/HoverEffectCompute.h"
#include "MouseFx/Core/Effects/ScrollEffectCompute.h"
#include "MouseFx/Core/Effects/TrailEffectCompute.h"
#include "Platform/macos/Effects/MacosEffectRenderProfile.h"

namespace mousefx::macos_effect_compute_profile {

ClickEffectProfile BuildClickProfile(const macos_effect_profile::ClickRenderProfile& profile);
TrailEffectProfile BuildTrailProfile(const macos_effect_profile::TrailRenderProfile& profile);
TrailEffectThrottleProfile BuildTrailThrottleProfile(const macos_effect_profile::TrailThrottleProfile& profile);
ScrollEffectProfile BuildScrollProfile(const macos_effect_profile::ScrollRenderProfile& profile);
HoverEffectProfile BuildHoverProfile(const macos_effect_profile::HoverRenderProfile& profile);
HoldEffectProfile BuildHoldProfile(const macos_effect_profile::HoldRenderProfile& profile);

} // namespace mousefx::macos_effect_compute_profile
