#pragma once

#include "MouseFx/Core/System/IGlobalMouseHook.h"

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <thread>

#if defined(__APPLE__)
#include <ApplicationServices/ApplicationServices.h>
#endif

namespace mousefx {

class IDispatchMessageHost;

class MacosGlobalInputHook final : public IGlobalMouseHook {
public:
    MacosGlobalInputHook() = default;
    ~MacosGlobalInputHook() override;

    MacosGlobalInputHook(const MacosGlobalInputHook&) = delete;
    MacosGlobalInputHook& operator=(const MacosGlobalInputHook&) = delete;

    bool Start(IDispatchMessageHost* dispatchHost) override;
    void Stop() override;
    uint32_t LastError() const override;
    bool ConsumeLatestMove(ScreenPoint& outPt) override;
    void SetKeyboardCaptureExclusive(bool enabled) override;

private:
    void RunEventTapLoop();
    void OnPermissionProbeTimer();
#if defined(__APPLE__)
    static CGEventRef EventTapCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void* userInfo);
    static void PermissionProbeTimerCallback(CFRunLoopTimerRef timer, void* userInfo);
#endif

private:
    IDispatchMessageHost* dispatchHost_ = nullptr;
    std::thread tapThread_{};
    std::atomic<bool> running_{false};
    std::atomic<uint32_t> lastError_{0};
    std::atomic<bool> keyboardCaptureExclusive_{false};
    std::atomic<bool> movePending_{false};
    std::atomic<int32_t> latestMoveX_{0};
    std::atomic<int32_t> latestMoveY_{0};

    std::mutex initMutex_{};
    std::condition_variable initCv_{};
    bool initDone_ = false;
    bool initOk_ = false;

    mutable std::mutex runLoopMutex_{};
    void* runLoopRef_ = nullptr;
    void* tapRef_ = nullptr;
    void* sourceRef_ = nullptr;
};

} // namespace mousefx
