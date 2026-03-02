#include "Platform/macos/Shell/MacosEventLoopService.h"

#include "Platform/macos/Shell/MacosEventLoopBridge.h"

#include <utility>

namespace mousefx {

#if defined(__APPLE__)

void MacosEventLoopService::RunLoopSourcePerform(void* info) {
    auto* self = static_cast<MacosEventLoopService*>(info);
    if (self != nullptr) {
        self->DrainTasksOnRunLoopThread();
    }
}

int MacosEventLoopService::Run() {
    platform::macos::EnsureMacosApplicationReady();

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (running_) {
            return -1;
        }

        running_ = true;
        runLoop_ = CFRunLoopGetCurrent();
        if (runLoop_ != nullptr) {
            CFRetain(runLoop_);
        }

        CFRunLoopSourceContext context{};
        context.version = 0;
        context.info = this;
        context.perform = &MacosEventLoopService::RunLoopSourcePerform;

        runLoopSource_ = CFRunLoopSourceCreate(kCFAllocatorDefault, 0, &context);
        if (runLoop_ == nullptr || runLoopSource_ == nullptr) {
            if (runLoopSource_ != nullptr) {
                CFRelease(runLoopSource_);
                runLoopSource_ = nullptr;
            }
            if (runLoop_ != nullptr) {
                CFRelease(runLoop_);
                runLoop_ = nullptr;
            }
            running_ = false;
            exitRequested_ = false;
            return -1;
        }

        CFRunLoopAddSource(runLoop_, runLoopSource_, kCFRunLoopDefaultMode);
        CFRunLoopAddSource(runLoop_, runLoopSource_, kCFRunLoopCommonModes);
        if (exitRequested_ || !taskQueue_.empty()) {
            SignalRunLoopLocked();
        }
    }

    platform::macos::RunMacosApplicationEventLoop();

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (runLoop_ != nullptr && runLoopSource_ != nullptr) {
            CFRunLoopRemoveSource(runLoop_, runLoopSource_, kCFRunLoopDefaultMode);
            CFRunLoopRemoveSource(runLoop_, runLoopSource_, kCFRunLoopCommonModes);
        }
        if (runLoopSource_ != nullptr) {
            CFRelease(runLoopSource_);
            runLoopSource_ = nullptr;
        }
        if (runLoop_ != nullptr) {
            CFRelease(runLoop_);
            runLoop_ = nullptr;
        }
        exitRequested_ = false;
        running_ = false;
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

void MacosEventLoopService::SignalRunLoopLocked() {
    if (runLoop_ == nullptr || runLoopSource_ == nullptr) {
        return;
    }
    CFRunLoopSourceSignal(runLoopSource_);
    CFRunLoopWakeUp(runLoop_);
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
