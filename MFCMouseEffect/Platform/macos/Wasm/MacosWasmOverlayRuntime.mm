#include "pch.h"

#include "Platform/macos/Wasm/MacosWasmOverlayRuntime.h"
#include "Platform/macos/Wasm/MacosWasmOverlayPolicy.h"

#if defined(__APPLE__)
#import <AppKit/AppKit.h>
#import <dispatch/dispatch.h>
#endif

#include <chrono>
#include <mutex>
#include <unordered_set>

namespace mousefx::platform::macos {

namespace {

#if defined(__APPLE__)
using SteadyClock = std::chrono::steady_clock;

std::mutex& WindowSetMutex() {
    static std::mutex mutex;
    return mutex;
}

std::unordered_set<NSWindow*>& WindowSet() {
    static std::unordered_set<NSWindow*> windows;
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

void RunWasmOverlayOnMainThreadSync(std::function<void()> task) {
#if !defined(__APPLE__)
    (void)task;
#else
    if (!task) {
        return;
    }
    std::function<void()> copiedTask = std::move(task);
    if ([NSThread isMainThread]) {
        copiedTask();
        return;
    }
    dispatch_sync(dispatch_get_main_queue(), ^{
      copiedTask();
    });
#endif
}

void RunWasmOverlayOnMainThreadAsync(std::function<void()> task) {
#if !defined(__APPLE__)
    (void)task;
#else
    if (!task) {
        return;
    }
    std::function<void()> copiedTask = std::move(task);
    dispatch_async(dispatch_get_main_queue(), ^{
      copiedTask();
    });
#endif
}

WasmOverlayAdmissionResult TryAcquireWasmOverlaySlot(WasmOverlayKind kind) {
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

void ReleaseWasmOverlaySlot() {
#if !defined(__APPLE__)
    return;
#else
    std::lock_guard<std::mutex> lock(WindowSetMutex());
    if (PendingOverlayCount() > 0) {
        PendingOverlayCount() -= 1;
    }
#endif
}

size_t GetWasmOverlayInFlightCount() {
#if !defined(__APPLE__)
    return 0;
#else
    std::lock_guard<std::mutex> lock(WindowSetMutex());
    return InFlightOverlayCountLocked();
#endif
}

WasmOverlayThrottleCounters GetWasmOverlayThrottleCounters() {
#if !defined(__APPLE__)
    return {};
#else
    std::lock_guard<std::mutex> lock(WindowSetMutex());
    return ThrottleCounters();
#endif
}

void RegisterWasmOverlayWindow(void* windowHandle) {
#if !defined(__APPLE__)
    (void)windowHandle;
#else
    NSWindow* window = reinterpret_cast<NSWindow*>(windowHandle);
    if (window == nil) {
        return;
    }
    std::lock_guard<std::mutex> lock(WindowSetMutex());
    if (PendingOverlayCount() > 0) {
        PendingOverlayCount() -= 1;
    }
    WindowSet().insert(window);
#endif
}

bool TakeWasmOverlayWindow(void* windowHandle) {
#if !defined(__APPLE__)
    (void)windowHandle;
    return false;
#else
    NSWindow* window = reinterpret_cast<NSWindow*>(windowHandle);
    if (window == nil) {
        return false;
    }
    std::lock_guard<std::mutex> lock(WindowSetMutex());
    auto& windows = WindowSet();
    const auto it = windows.find(window);
    if (it == windows.end()) {
        return false;
    }
    windows.erase(it);
    return true;
#endif
}

void CloseAllWasmOverlayWindows() {
#if !defined(__APPLE__)
    return;
#else
    RunWasmOverlayOnMainThreadSync([] {
      std::unordered_set<NSWindow*> windows;
      {
          std::lock_guard<std::mutex> lock(WindowSetMutex());
          PendingOverlayCount() = 0;
          LastImageAdmitTime() = SteadyClock::time_point{};
          LastTextAdmitTime() = SteadyClock::time_point{};
          ThrottleCounters() = WasmOverlayThrottleCounters{};
          windows.swap(WindowSet());
      }
      for (NSWindow* window : windows) {
          if (window == nil) {
              continue;
          }
          [window orderOut:nil];
          [window release];
      }
    });
#endif
}

} // namespace mousefx::platform::macos
