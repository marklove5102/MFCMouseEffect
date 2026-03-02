#include "pch.h"

#include "Platform/macos/Control/MacosDispatchMessageHost.h"

#include "MouseFx/Core/Control/IDispatchMessageHandler.h"
#include "Platform/macos/Control/MacosDispatchMessageCodec.h"

#include <chrono>
#include <future>
#include <utility>

namespace mousefx {

namespace {

constexpr uint32_t kErrorSuccess = 0;
constexpr uint32_t kErrorInvalidParameter = 22;
constexpr uint32_t kErrorInvalidState = 125;

} // namespace

MacosDispatchMessageHost::~MacosDispatchMessageHost() {
    Destroy();
}

bool MacosDispatchMessageHost::Create(IDispatchMessageHandler* handler) {
    if (IsCreated()) {
        return true;
    }
    if (!handler) {
        lastError_.store(kErrorInvalidParameter, std::memory_order_release);
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        handler_ = handler;
        queue_.clear();
    }

    running_.store(true, std::memory_order_release);
    workerThread_ = std::thread([this]() { WorkerLoop(); });
    lastError_.store(kErrorSuccess, std::memory_order_release);
    return true;
}

void MacosDispatchMessageHost::Destroy() {
    if (!running_.exchange(false, std::memory_order_acq_rel)) {
        return;
    }

    StopAllTimers();
    cv_.notify_all();

    if (workerThread_.joinable()) {
        workerThread_.join();
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.clear();
        handler_ = nullptr;
        workerThreadId_ = std::thread::id{};
    }

    lastError_.store(kErrorSuccess, std::memory_order_release);
}

bool MacosDispatchMessageHost::IsCreated() const {
    return running_.load(std::memory_order_acquire) && workerThread_.joinable();
}

bool MacosDispatchMessageHost::IsOwnerThread() const {
    return IsCreated() && std::this_thread::get_id() == workerThreadId_;
}

uintptr_t MacosDispatchMessageHost::NativeHandle() const {
    return reinterpret_cast<uintptr_t>(this);
}

uint32_t MacosDispatchMessageHost::LastError() const {
    return lastError_.load(std::memory_order_acquire);
}

intptr_t MacosDispatchMessageHost::SendSync(uint32_t msg, uintptr_t wParam, intptr_t lParam) {
    if (!IsCreated()) {
        lastError_.store(kErrorInvalidState, std::memory_order_release);
        return 0;
    }
    if (IsOwnerThread()) {
        return DispatchMessageOnWorker(msg, wParam, lParam);
    }

    auto promise = std::make_shared<std::promise<intptr_t>>();
    std::future<intptr_t> future = promise->get_future();
    PendingMessage pending{};
    pending.msg = msg;
    pending.wParam = wParam;
    pending.lParam = lParam;
    pending.syncResult = promise;
    if (!EnqueueMessage(std::move(pending))) {
        return 0;
    }

    return future.get();
}

bool MacosDispatchMessageHost::PostAsync(uint32_t msg, uintptr_t wParam, intptr_t lParam) {
    if (!IsCreated()) {
        lastError_.store(kErrorInvalidState, std::memory_order_release);
        return false;
    }
    PendingMessage pending{};
    pending.msg = msg;
    pending.wParam = wParam;
    pending.lParam = lParam;
    return EnqueueMessage(std::move(pending));
}

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
    slot.running = std::make_shared<std::atomic<bool>>(true);
    slot.thread = std::thread([this, timerId, intervalMs, running = slot.running]() {
        const std::chrono::milliseconds interval(intervalMs);
        while (running->load(std::memory_order_acquire)) {
            std::this_thread::sleep_for(interval);
            if (!running->load(std::memory_order_acquire)) {
                break;
            }
            PostAsync(
                MacosDispatchMessageCodec::kTimerMessageId,
                static_cast<uintptr_t>(timerId),
                0);
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

    if (slot.running) {
        slot.running->store(false, std::memory_order_release);
    }
    if (slot.thread.joinable()) {
        slot.thread.join();
    }
}

void MacosDispatchMessageHost::WorkerLoop() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        workerThreadId_ = std::this_thread::get_id();
    }

    while (running_.load(std::memory_order_acquire)) {
        PendingMessage message{};
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this]() {
                return !running_.load(std::memory_order_acquire) || !queue_.empty();
            });
            if (!running_.load(std::memory_order_acquire) && queue_.empty()) {
                return;
            }
            message = std::move(queue_.front());
            queue_.pop_front();
        }

        const intptr_t result = DispatchMessageOnWorker(message.msg, message.wParam, message.lParam);
        if (message.syncResult) {
            message.syncResult->set_value(result);
        }
    }
}

intptr_t MacosDispatchMessageHost::DispatchMessageOnWorker(uint32_t msg, uintptr_t wParam, intptr_t lParam) {
    IDispatchMessageHandler* handler = nullptr;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        handler = handler_;
    }
    if (!handler) {
        lastError_.store(kErrorInvalidState, std::memory_order_release);
        return 0;
    }
    lastError_.store(kErrorSuccess, std::memory_order_release);
    return handler->OnDispatchMessage(NativeHandle(), msg, wParam, lParam);
}

bool MacosDispatchMessageHost::EnqueueMessage(PendingMessage message) {
    if (!running_.load(std::memory_order_acquire)) {
        lastError_.store(kErrorInvalidState, std::memory_order_release);
        return false;
    }
    {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push_back(std::move(message));
    }
    cv_.notify_one();
    return true;
}

void MacosDispatchMessageHost::StopAllTimers() {
    std::unordered_map<uintptr_t, TimerThreadSlot> timers;
    {
        std::lock_guard<std::mutex> lock(timerMutex_);
        timers.swap(timers_);
    }

    for (auto& entry : timers) {
        TimerThreadSlot& slot = entry.second;
        if (slot.running) {
            slot.running->store(false, std::memory_order_release);
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
