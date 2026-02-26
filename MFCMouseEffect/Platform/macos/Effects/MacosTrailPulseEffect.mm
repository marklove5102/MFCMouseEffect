#include "pch.h"

#include "Platform/macos/Effects/MacosTrailPulseEffect.h"

#include "MouseFx/Core/Overlay/OverlayCoordSpace.h"
#include "MouseFx/Utils/StringUtils.h"
#include "Platform/macos/Effects/MacosTrailPulseOverlayRenderer.h"

#include <chrono>
#include <cmath>
#include <utility>

namespace mousefx {
namespace {

struct TrailThrottleProfile {
    uint64_t minIntervalMs = 18;
    double minDistancePx = 8.0;
};

TrailThrottleProfile ResolveTrailProfile(const std::string& trailType) {
    const std::string type = ToLowerAscii(trailType);
    if (type == "particle") {
        return {10, 3.0};
    }
    if (type == "meteor") {
        return {14, 5.0};
    }
    if (type == "streamer") {
        return {12, 4.0};
    }
    if (type == "electric") {
        return {15, 6.0};
    }
    if (type == "tubes") {
        return {18, 8.0};
    }
    return {};
}

} // namespace

MacosTrailPulseEffect::MacosTrailPulseEffect(std::string effectType, std::string themeName)
    : effectType_(std::move(effectType)),
      themeName_(std::move(themeName)) {
    if (effectType_.empty()) {
        effectType_ = "line";
    }
}

MacosTrailPulseEffect::~MacosTrailPulseEffect() {
    Shutdown();
}

bool MacosTrailPulseEffect::Initialize() {
    initialized_ = true;
    hasLastPoint_ = false;
    lastEmitTickMs_ = 0;
    return true;
}

void MacosTrailPulseEffect::Shutdown() {
    initialized_ = false;
    hasLastPoint_ = false;
    lastEmitTickMs_ = 0;
    macos_trail_pulse::CloseAllTrailPulseWindows();
}

void MacosTrailPulseEffect::OnMouseMove(const ScreenPoint& pt) {
    if (!initialized_) {
        return;
    }

    if (!hasLastPoint_) {
        hasLastPoint_ = true;
        lastPoint_ = pt;
        return;
    }

    const double dx = static_cast<double>(pt.x - lastPoint_.x);
    const double dy = static_cast<double>(pt.y - lastPoint_.y);
    const double distance = std::sqrt(dx * dx + dy * dy);
    const TrailThrottleProfile profile = ResolveTrailProfile(effectType_);

    const uint64_t now = CurrentTickMs();
    if (distance < profile.minDistancePx) {
        return;
    }
    if (lastEmitTickMs_ != 0 && now - lastEmitTickMs_ < profile.minIntervalMs) {
        return;
    }

    lastEmitTickMs_ = now;
    lastPoint_ = pt;

    const ScreenPoint overlayPt = ScreenToOverlayPoint(pt);
    macos_trail_pulse::ShowTrailPulseOverlay(overlayPt, dx, dy, effectType_, themeName_);
}

uint64_t MacosTrailPulseEffect::CurrentTickMs() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

} // namespace mousefx
