#include "pch.h"

#include "Platform/macos/System/MacosGlobalInputHook.h"

#include "Platform/macos/System/MacosInputPermissionState.h"

#include <chrono>
#include <string>
#include <thread>

namespace mousefx {

void MacosGlobalInputHook::RunEventTapLoop() {
#if !defined(__APPLE__)
    lastError_.store(kErrorTapCreateFailed, std::memory_order_release);
    {
        std::lock_guard<std::mutex> lock(initMutex_);
        initOk_ = false;
        initDone_ = true;
    }
    initCv_.notify_all();
    return;
#else
    const std::string simulationFilePath = macos_input_permission::ReadPermissionSimulationFilePath();
    const bool trusted = macos_input_permission::IsRuntimeInputTrusted(simulationFilePath);

    if (!simulationFilePath.empty()) {
        if (!trusted) {
            lastError_.store(kErrorPermissionDenied, std::memory_order_release);
            {
                std::lock_guard<std::mutex> lock(initMutex_);
                initOk_ = false;
                initDone_ = true;
            }
            initCv_.notify_all();
            return;
        }

        lastError_.store(kErrorSuccess, std::memory_order_release);
        {
            std::lock_guard<std::mutex> lock(initMutex_);
            initOk_ = true;
            initDone_ = true;
        }
        initCv_.notify_all();

        while (running_.load(std::memory_order_acquire)) {
            const bool runtimeTrusted = macos_input_permission::IsRuntimeInputTrusted(simulationFilePath);
            lastError_.store(runtimeTrusted ? kErrorSuccess : kErrorPermissionDenied, std::memory_order_release);
            std::this_thread::sleep_for(std::chrono::milliseconds(kPermissionSimulationPollIntervalMs));
        }
        return;
    }

    const CGEventMask mask =
        CGEventMaskBit(kCGEventMouseMoved) |
        CGEventMaskBit(kCGEventLeftMouseDown) |
        CGEventMaskBit(kCGEventLeftMouseUp) |
        CGEventMaskBit(kCGEventRightMouseDown) |
        CGEventMaskBit(kCGEventRightMouseUp) |
        CGEventMaskBit(kCGEventOtherMouseDown) |
        CGEventMaskBit(kCGEventOtherMouseUp) |
        CGEventMaskBit(kCGEventLeftMouseDragged) |
        CGEventMaskBit(kCGEventRightMouseDragged) |
        CGEventMaskBit(kCGEventOtherMouseDragged) |
        CGEventMaskBit(kCGEventScrollWheel) |
        CGEventMaskBit(kCGEventKeyDown);

    CFMachPortRef tap = CGEventTapCreate(
        kCGSessionEventTap,
        kCGHeadInsertEventTap,
        kCGEventTapOptionDefault,
        mask,
        MacosGlobalInputHook::EventTapCallback,
        this);
    if (!tap) {
        lastError_.store(trusted ? kErrorTapCreateFailed : kErrorPermissionDenied, std::memory_order_release);
        {
            std::lock_guard<std::mutex> lock(initMutex_);
            initOk_ = false;
            initDone_ = true;
        }
        initCv_.notify_all();
        return;
    }

    CFRunLoopSourceRef source = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, tap, 0);
    if (!source) {
        CFRelease(tap);
        lastError_.store(kErrorSourceCreateFailed, std::memory_order_release);
        {
            std::lock_guard<std::mutex> lock(initMutex_);
            initOk_ = false;
            initDone_ = true;
        }
        initCv_.notify_all();
        return;
    }

    CFRunLoopRef runLoop = CFRunLoopGetCurrent();
    CFRunLoopTimerRef permissionProbeTimer = nullptr;
    CFRunLoopTimerContext timerContext{};
    timerContext.info = this;
    permissionProbeTimer = CFRunLoopTimerCreate(
        kCFAllocatorDefault,
        CFAbsoluteTimeGetCurrent() + kPermissionProbeIntervalSeconds,
        kPermissionProbeIntervalSeconds,
        0,
        0,
        MacosGlobalInputHook::PermissionProbeTimerCallback,
        &timerContext);
    {
        std::lock_guard<std::mutex> lock(runLoopMutex_);
        runLoopRef_ = runLoop;
        tapRef_ = tap;
        sourceRef_ = source;
    }

    CFRunLoopAddSource(runLoop, source, kCFRunLoopCommonModes);
    if (permissionProbeTimer != nullptr) {
        CFRunLoopAddTimer(runLoop, permissionProbeTimer, kCFRunLoopCommonModes);
    }
    CGEventTapEnable(tap, true);

    {
        std::lock_guard<std::mutex> lock(initMutex_);
        initOk_ = true;
        initDone_ = true;
    }
    initCv_.notify_all();

    CFRunLoopRun();

    if (permissionProbeTimer != nullptr) {
        CFRunLoopRemoveTimer(runLoop, permissionProbeTimer, kCFRunLoopCommonModes);
    }
    CFRunLoopRemoveSource(runLoop, source, kCFRunLoopCommonModes);
    {
        std::lock_guard<std::mutex> lock(runLoopMutex_);
        runLoopRef_ = nullptr;
        tapRef_ = nullptr;
        sourceRef_ = nullptr;
    }
    if (permissionProbeTimer != nullptr) {
        CFRelease(permissionProbeTimer);
    }
    CFRelease(source);
    CFRelease(tap);
#endif
}

} // namespace mousefx
