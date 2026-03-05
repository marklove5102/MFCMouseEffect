#include "pch.h"

#include "Platform/macos/Control/MacosDispatchMessageHost.h"

#include "Platform/macos/Control/MacosDispatchMessageCodec.h"

#include <chrono>

namespace mousefx {

bool MacosDispatchMessageHost::SetTimer(uintptr_t timerId, uint32_t intervalMs) {
    if (!IsCreated()) {
        lastError_.store(kErrorInvalidState, std::memory_order_release);
        return false;
    }
    if (timerId == 0 || intervalMs == 0) {
        lastError_.store(kErrorInvalidParameter, std::memory_order_release);
        return false;
    }

    KillTimer(timerId);

    TimerThreadSlot slot{};
    slot.stopSignal = std::make_shared<TimerThreadSlot::StopSignal>();
    slot.thread = std::thread([this, timerId, intervalMs, stopSignal = slot.stopSignal]() {
        const std::chrono::milliseconds interval(intervalMs);
        std::unique_lock<std::mutex> lock(stopSignal->mutex);
        for (;;) {
            if (stopSignal->cv.wait_for(
                    lock,
                    interval,
                    [stopSignal]() { return stopSignal->stop; })) {
                break;
            }
            lock.unlock();
            PostAsync(
                MacosDispatchMessageCodec::kTimerMessageId,
                static_cast<uintptr_t>(timerId),
                0);
            lock.lock();
        }
    });

    {
        std::lock_guard<std::mutex> lock(timerMutex_);
        timers_.emplace(timerId, std::move(slot));
    }
    lastError_.store(kErrorSuccess, std::memory_order_release);
    return true;
}

void MacosDispatchMessageHost::KillTimer(uintptr_t timerId) {
    TimerThreadSlot slot{};
    bool found = false;
    {
        std::lock_guard<std::mutex> lock(timerMutex_);
        const auto it = timers_.find(timerId);
        if (it != timers_.end()) {
            slot = std::move(it->second);
            timers_.erase(it);
            found = true;
        }
    }
    if (!found) {
        return;
    }

    if (slot.stopSignal) {
        {
            std::lock_guard<std::mutex> lock(slot.stopSignal->mutex);
            slot.stopSignal->stop = true;
        }
        slot.stopSignal->cv.notify_all();
    }
    if (slot.thread.joinable()) {
        slot.thread.join();
    }
}

void MacosDispatchMessageHost::StopAllTimers() {
    std::unordered_map<uintptr_t, TimerThreadSlot> timers;
    {
        std::lock_guard<std::mutex> lock(timerMutex_);
        timers.swap(timers_);
    }

    for (auto& entry : timers) {
        TimerThreadSlot& slot = entry.second;
        if (slot.stopSignal) {
            {
                std::lock_guard<std::mutex> lock(slot.stopSignal->mutex);
                slot.stopSignal->stop = true;
            }
            slot.stopSignal->cv.notify_all();
        }
    }
    for (auto& entry : timers) {
        TimerThreadSlot& slot = entry.second;
        if (slot.thread.joinable()) {
            slot.thread.join();
        }
    }
}

} // namespace mousefx
