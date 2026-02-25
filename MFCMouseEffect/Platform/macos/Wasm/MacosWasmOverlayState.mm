#include "pch.h"

#include "Platform/macos/Wasm/MacosWasmOverlayState.h"

#include "Platform/macos/Wasm/MacosWasmOverlayPolicy.h"

#if defined(__APPLE__)
#import <AppKit/AppKit.h>
#endif

#include <chrono>
#include <mutex>
#include <unordered_set>
#include <vector>

namespace mousefx::platform::macos {
namespace {

#if defined(__APPLE__)
using SteadyClock = std::chrono::steady_clock;

std::mutex& WindowSetMutex() {
    static std::mutex mutex;
    return mutex;
}

std::unordered_set<void*>& WindowSet() {
    static std::unordered_set<void*> windows;
    return windows;
}

size_t& PendingOverlayCount() {
    static size_t count = 0;
    return count;
}

SteadyClock::time_point& LastImageAdmitTime() {
    static SteadyClock::time_point last;
    return last;
}

SteadyClock::time_point& LastTextAdmitTime() {
    static SteadyClock::time_point last;
    return last;
}

SteadyClock::time_point& LastAdmitTime(WasmOverlayKind kind) {
    return (kind == WasmOverlayKind::Image) ? LastImageAdmitTime() : LastTextAdmitTime();
}

uint32_t MinIntervalMs(const MacosWasmOverlayPolicy& policy, WasmOverlayKind kind) {
    return (kind == WasmOverlayKind::Image) ? policy.minImageIntervalMs : policy.minTextIntervalMs;
}

size_t InFlightOverlayCountLocked() {
    return WindowSet().size() + PendingOverlayCount();
}

WasmOverlayThrottleCounters& ThrottleCounters() {
    static WasmOverlayThrottleCounters counters;
    return counters;
}
#endif

} // namespace

WasmOverlayAdmissionResult TryAcquireWasmOverlaySlotState(WasmOverlayKind kind) {
#if !defined(__APPLE__)
    (void)kind;
    return WasmOverlayAdmissionResult::RejectedByCapacity;
#else
    const MacosWasmOverlayPolicy& policy = GetMacosWasmOverlayPolicy();
    const SteadyClock::time_point now = SteadyClock::now();
    std::lock_guard<std::mutex> lock(WindowSetMutex());
    if (InFlightOverlayCountLocked() >= policy.maxInFlightOverlays) {
        ThrottleCounters().rejectedByCapacity += 1;
        return WasmOverlayAdmissionResult::RejectedByCapacity;
    }

    const uint32_t minIntervalMs = MinIntervalMs(policy, kind);
    if (minIntervalMs > 0) {
        SteadyClock::time_point& last = LastAdmitTime(kind);
        if (last.time_since_epoch().count() != 0) {
            const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last);
            if (elapsed < std::chrono::milliseconds(minIntervalMs)) {
                if (kind == WasmOverlayKind::Image) {
                    ThrottleCounters().rejectedByImageInterval += 1;
                } else {
                    ThrottleCounters().rejectedByTextInterval += 1;
                }
                return WasmOverlayAdmissionResult::RejectedByInterval;
            }
        }
        last = now;
    }

    PendingOverlayCount() += 1;
    return WasmOverlayAdmissionResult::Accepted;
#endif
}

void ReleaseWasmOverlaySlotState() {
#if !defined(__APPLE__)
    return;
#else
    std::lock_guard<std::mutex> lock(WindowSetMutex());
    if (PendingOverlayCount() > 0) {
        PendingOverlayCount() -= 1;
    }
#endif
}

size_t GetWasmOverlayInFlightCountState() {
#if !defined(__APPLE__)
    return 0;
#else
    std::lock_guard<std::mutex> lock(WindowSetMutex());
    return InFlightOverlayCountLocked();
#endif
}

WasmOverlayThrottleCounters GetWasmOverlayThrottleCountersState() {
#if !defined(__APPLE__)
    return {};
#else
    std::lock_guard<std::mutex> lock(WindowSetMutex());
    return ThrottleCounters();
#endif
}

void RegisterWasmOverlayWindowState(void* windowHandle) {
#if !defined(__APPLE__)
    (void)windowHandle;
#else
    if (windowHandle == nullptr) {
        return;
    }
    std::lock_guard<std::mutex> lock(WindowSetMutex());
    if (PendingOverlayCount() > 0) {
        PendingOverlayCount() -= 1;
    }
    WindowSet().insert(windowHandle);
#endif
}

bool TakeWasmOverlayWindowState(void* windowHandle) {
#if !defined(__APPLE__)
    (void)windowHandle;
    return false;
#else
    if (windowHandle == nullptr) {
        return false;
    }
    std::lock_guard<std::mutex> lock(WindowSetMutex());
    auto& windows = WindowSet();
    const auto it = windows.find(windowHandle);
    if (it == windows.end()) {
        return false;
    }
    windows.erase(it);
    return true;
#endif
}

std::vector<void*> ResetAndTakeAllWasmOverlayWindowsState() {
#if !defined(__APPLE__)
    return {};
#else
    std::vector<void*> windows;
    std::lock_guard<std::mutex> lock(WindowSetMutex());
    PendingOverlayCount() = 0;
    LastImageAdmitTime() = SteadyClock::time_point{};
    LastTextAdmitTime() = SteadyClock::time_point{};
    ThrottleCounters() = WasmOverlayThrottleCounters{};
    windows.reserve(WindowSet().size());
    for (void* window : WindowSet()) {
        windows.push_back(window);
    }
    WindowSet().clear();
    return windows;
#endif
}

} // namespace mousefx::platform::macos
