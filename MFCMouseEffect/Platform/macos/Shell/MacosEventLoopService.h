#pragma once

#include "MouseFx/Core/Shell/IEventLoopService.h"

#include <functional>
#include <mutex>
#include <queue>

#if defined(__APPLE__)
#include <CoreFoundation/CoreFoundation.h>
#else
#include "Platform/posix/Shell/PosixBlockingEventLoop.h"
#endif

namespace mousefx {

class MacosEventLoopService final : public IEventLoopService {
public:
    int Run() override;
    void RequestExit() override;
    bool PostTask(std::function<void()> task) override;

private:
#if defined(__APPLE__)
    static void RunLoopSourcePerform(void* info);

    void DrainTasksOnRunLoopThread();
    void SignalRunLoopLocked();

    std::mutex mutex_{};
    std::queue<std::function<void()>> taskQueue_{};
    bool running_ = false;
    bool exitRequested_ = false;
    CFRunLoopRef runLoop_ = nullptr;
    CFRunLoopSourceRef runLoopSource_ = nullptr;
#else
    PosixBlockingEventLoop loop_{};
#endif
};

} // namespace mousefx
