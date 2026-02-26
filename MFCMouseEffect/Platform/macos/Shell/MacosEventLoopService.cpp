#include "Platform/macos/Shell/MacosEventLoopService.h"

#include "Platform/macos/Shell/MacosEventLoopBridge.h"

#include <utility>

namespace mousefx {

#if defined(__APPLE__)

int MacosEventLoopService::Run() {
    platform::macos::EnsureMacosApplicationReady();

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (running_) {
            return -1;
        }

        if (!SetupRunLoopLocked()) {
            return -1;
        }
        if (exitRequested_ || !taskQueue_.empty()) {
            SignalRunLoopLocked();
        }
    }

    platform::macos::RunMacosApplicationEventLoop();

    {
        std::lock_guard<std::mutex> lock(mutex_);
        TeardownRunLoopLocked();
    }

    return 0;
}

void MacosEventLoopService::RequestExit() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        exitRequested_ = true;
        SignalRunLoopLocked();
    }
}

bool MacosEventLoopService::PostTask(std::function<void()> task) {
    if (!task) {
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        taskQueue_.push(std::move(task));
        SignalRunLoopLocked();
    }

    return true;
}

void MacosEventLoopService::DrainTasksOnRunLoopThread() {
    for (;;) {
        std::function<void()> task;
        bool shouldExit = false;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!taskQueue_.empty()) {
                task = std::move(taskQueue_.front());
                taskQueue_.pop();
            } else {
                shouldExit = exitRequested_;
            }
        }

        if (task) {
            task();
            continue;
        }

        if (shouldExit) {
            platform::macos::StopMacosApplicationEventLoop();
        }
        break;
    }
}

#else

int MacosEventLoopService::Run() {
    return loop_.Run();
}

void MacosEventLoopService::RequestExit() {
    loop_.RequestExit();
}

bool MacosEventLoopService::PostTask(std::function<void()> task) {
    return loop_.PostTask(std::move(task));
}

#endif

} // namespace mousefx
