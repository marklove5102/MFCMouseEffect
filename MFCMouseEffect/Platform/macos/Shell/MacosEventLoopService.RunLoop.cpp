#include "Platform/macos/Shell/MacosEventLoopService.h"

namespace mousefx {

#if defined(__APPLE__)

void MacosEventLoopService::RunLoopSourcePerform(void* info) {
    auto* self = static_cast<MacosEventLoopService*>(info);
    if (self != nullptr) {
        self->DrainTasksOnRunLoopThread();
    }
}

bool MacosEventLoopService::SetupRunLoopLocked() {
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
        TeardownRunLoopLocked();
        return false;
    }

    CFRunLoopAddSource(runLoop_, runLoopSource_, kCFRunLoopDefaultMode);
    CFRunLoopAddSource(runLoop_, runLoopSource_, kCFRunLoopCommonModes);
    return true;
}

void MacosEventLoopService::TeardownRunLoopLocked() {
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

void MacosEventLoopService::SignalRunLoopLocked() {
    if (runLoop_ == nullptr || runLoopSource_ == nullptr) {
        return;
    }
    CFRunLoopSourceSignal(runLoopSource_);
    CFRunLoopWakeUp(runLoop_);
}

#endif

} // namespace mousefx
