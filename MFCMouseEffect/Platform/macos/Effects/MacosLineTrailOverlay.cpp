#include "pch.h"

#include "Platform/macos/Effects/MacosLineTrailOverlay.h"

#include "MouseFx/Core/Overlay/OverlayCoordSpace.h"
#include "Platform/macos/Effects/MacosLineTrailOverlaySwiftBridge.h"

#include <algorithm>
#include <chrono>
#include <mutex>

namespace mousefx::macos_line_trail {

#if defined(__APPLE__)
namespace {

uint64_t MonotonicNowMs() {
    using namespace std::chrono;
    return static_cast<uint64_t>(
        duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
}

class LineTrailBridgeState final {
public:
    ~LineTrailBridgeState() {
        Release();
    }

    void Update(const ScreenPoint& overlayPt, const LineTrailConfig& config) {
        std::lock_guard<std::mutex> lock(mutex_);
        EnsureHandleLocked();
        if (handle_ == nullptr) {
            return;
        }
        const uint64_t nowMs = MonotonicNowMs();
        if (lastUpdateDispatchTickMs_ != 0 &&
            nowMs >= lastUpdateDispatchTickMs_ &&
            (nowMs - lastUpdateDispatchTickMs_) < kMinDispatchIntervalMs) {
            return;
        }
        lastUpdateDispatchTickMs_ = nowMs;

        const int durationMs = std::max(1, config.durationMs);
        const float lineWidth = std::max(0.2f, config.lineWidth);
        const int idleFadeStartMs = std::max(0, config.idleFade.startMs);
        const int idleFadeEndMs = std::max(idleFadeStartMs + 1, config.idleFade.endMs);

        mfx_macos_line_trail_update_v1(
            handle_,
            overlayPt.x,
            overlayPt.y,
            durationMs,
            lineWidth,
            config.strokeArgb,
            config.fillArgb,
            static_cast<int>(config.style),
            std::clamp(config.intensity, 0.0, 1.0),
            config.chromatic ? 1 : 0,
            std::clamp(config.streamerGlowWidthScale, 0.5f, 4.0f),
            std::clamp(config.streamerCoreWidthScale, 0.2f, 2.0f),
            std::clamp(config.streamerHeadPower, 0.8f, 3.0f),
            std::clamp(config.electricAmplitudeScale, 0.2f, 3.0f),
            std::clamp(config.electricForkChance, 0.0f, 0.5f),
            std::clamp(config.meteorSparkRateScale, 0.2f, 4.0f),
            std::clamp(config.meteorSparkSpeedScale, 0.2f, 4.0f),
            idleFadeStartMs,
            idleFadeEndMs);
    }

    void Reset() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (handle_ == nullptr) {
            return;
        }
        mfx_macos_line_trail_reset_v1(handle_);
    }

    LineTrailRuntimeSnapshot Snapshot() {
        std::lock_guard<std::mutex> lock(mutex_);
        LineTrailRuntimeSnapshot snapshot{};
        if (handle_ == nullptr) {
            return snapshot;
        }
        snapshot.active = (mfx_macos_line_trail_is_active_v1(handle_) != 0);
        snapshot.pointCount = std::max(0, mfx_macos_line_trail_point_count_v1(handle_));
        snapshot.lineWidthPx = std::max(0.0, mfx_macos_line_trail_line_width_px_v1(handle_));
        return snapshot;
    }

private:
    void EnsureHandleLocked() {
        if (handle_ != nullptr) {
            return;
        }
        handle_ = mfx_macos_line_trail_create_v1();
    }

    void Release() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (handle_ == nullptr) {
            return;
        }
        mfx_macos_line_trail_release_v1(handle_);
        handle_ = nullptr;
    }

    std::mutex mutex_{};
    void* handle_ = nullptr;
    uint64_t lastUpdateDispatchTickMs_ = 0;
    static constexpr uint64_t kMinDispatchIntervalMs = 10;
};

LineTrailBridgeState& BridgeState() {
    static LineTrailBridgeState state{};
    return state;
}

} // namespace
#endif

void UpdateLineTrail(const ScreenPoint& screenPt, const LineTrailConfig& config) {
#if !defined(__APPLE__)
    (void)screenPt;
    (void)config;
#else
    const ScreenPoint overlayPt = ScreenToOverlayPoint(screenPt);
    BridgeState().Update(overlayPt, config);
#endif
}

void ResetLineTrail() {
#if defined(__APPLE__)
    BridgeState().Reset();
#endif
}

LineTrailRuntimeSnapshot ReadLineTrailRuntimeSnapshot() {
    LineTrailRuntimeSnapshot snapshot{};
#if defined(__APPLE__)
    snapshot = BridgeState().Snapshot();
#endif
    return snapshot;
}

} // namespace mousefx::macos_line_trail
