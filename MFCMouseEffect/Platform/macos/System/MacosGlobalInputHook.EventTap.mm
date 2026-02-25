#include "pch.h"

#include "Platform/macos/System/MacosGlobalInputHook.h"

#include "MouseFx/Core/Control/IDispatchMessageHost.h"
#include "MouseFx/Core/Protocol/MouseFxMessages.h"
#include "Platform/macos/Control/MacosDispatchMessageCodec.h"
#include "Platform/macos/System/MacosInputEventUtils.h"
#include "Platform/macos/System/MacosInputPermissionState.h"
#include "Platform/macos/System/MacosVirtualKeyMapper.h"

#include <new>
#include <string>

namespace mousefx {

#if defined(__APPLE__)
CGEventRef MacosGlobalInputHook::EventTapCallback(CGEventTapProxy proxy,
                                                   CGEventType type,
                                                   CGEventRef event,
                                                   void* userInfo) {
    (void)proxy;
    auto* self = reinterpret_cast<MacosGlobalInputHook*>(userInfo);
    if (!self || !event) {
        return event;
    }

    if (type == kCGEventTapDisabledByTimeout || type == kCGEventTapDisabledByUserInput) {
        const bool trusted = AXIsProcessTrusted();
        if (!trusted) {
            self->lastError_.store(kErrorPermissionDenied, std::memory_order_release);
            return event;
        }

        CFMachPortRef tap = nullptr;
        {
            std::lock_guard<std::mutex> lock(self->runLoopMutex_);
            tap = static_cast<CFMachPortRef>(self->tapRef_);
        }
        if (tap) {
            CGEventTapEnable(tap, true);
            self->lastError_.store(kErrorSuccess, std::memory_order_release);
        }
        return event;
    }

    if (!self->running_.load(std::memory_order_acquire) || !self->dispatchHost_) {
        return event;
    }
    self->lastError_.store(kErrorSuccess, std::memory_order_release);

    if (macos_input_event::IsMouseMoveType(type)) {
        const ScreenPoint pt = macos_input_event::ToScreenPoint(CGEventGetLocation(event));
        self->latestMoveX_.store(pt.x, std::memory_order_release);
        self->latestMoveY_.store(pt.y, std::memory_order_release);
        if (!self->movePending_.exchange(true, std::memory_order_acq_rel)) {
            if (!self->dispatchHost_->PostAsync(
                    WM_MFX_MOVE,
                    static_cast<uintptr_t>(pt.x),
                    static_cast<intptr_t>(pt.y))) {
                self->movePending_.store(false, std::memory_order_release);
            }
        }
        return event;
    }

    if (type == kCGEventScrollWheel) {
        const int32_t delta = static_cast<int32_t>(
            CGEventGetIntegerValueField(event, kCGScrollWheelEventDeltaAxis1));
        if (delta != 0) {
            const ScreenPoint pt = macos_input_event::ToScreenPoint(CGEventGetLocation(event));
            self->dispatchHost_->PostAsync(
                WM_MFX_SCROLL,
                static_cast<uintptr_t>(delta),
                MacosDispatchMessageCodec::PackPointPayload(pt.x, pt.y));
        }
        return event;
    }

    if (macos_input_event::IsMouseDownType(type)) {
        const ScreenPoint pt = macos_input_event::ToScreenPoint(CGEventGetLocation(event));
        const MouseButton button = macos_input_event::MouseButtonFromEvent(event);
        self->dispatchHost_->PostAsync(
            WM_MFX_BUTTON_DOWN,
            static_cast<uintptr_t>(button),
            MacosDispatchMessageCodec::PackPointPayload(pt.x, pt.y));
        return event;
    }

    if (macos_input_event::IsMouseUpType(type)) {
        const ScreenPoint pt = macos_input_event::ToScreenPoint(CGEventGetLocation(event));
        const MouseButton button = macos_input_event::MouseButtonFromEvent(event);
        self->dispatchHost_->PostAsync(
            WM_MFX_BUTTON_UP,
            static_cast<uintptr_t>(button),
            0);

        auto* click = new (std::nothrow) ClickEvent();
        if (click) {
            click->pt = pt;
            click->button = button;
            if (!self->dispatchHost_->PostAsync(
                    WM_MFX_CLICK,
                    0,
                    reinterpret_cast<intptr_t>(click))) {
                delete click;
            }
        }
        return event;
    }

    if (type == kCGEventKeyDown) {
        auto* key = new (std::nothrow) KeyEvent();
        if (key) {
            const uint16_t macKeyCode = static_cast<uint16_t>(
                CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode));
            key->vkCode = macos_keymap::VirtualKeyFromMacKeyCode(macKeyCode);
            key->pt = macos_input_event::ToScreenPoint(CGEventGetLocation(event));
            const CGEventFlags flags = CGEventGetFlags(event);
            key->ctrl = (flags & kCGEventFlagMaskControl) != 0;
            key->shift = (flags & kCGEventFlagMaskShift) != 0;
            key->alt = (flags & kCGEventFlagMaskAlternate) != 0;
            key->win = (flags & kCGEventFlagMaskCommand) != 0;
            key->meta = key->win;
            key->systemKey = key->alt || key->meta;
            if (!self->dispatchHost_->PostAsync(
                    WM_MFX_KEY,
                    0,
                    reinterpret_cast<intptr_t>(key))) {
                delete key;
            }
        }

        if (self->keyboardCaptureExclusive_.load(std::memory_order_acquire)) {
            return nullptr;
        }
        return event;
    }

    return event;
}

void MacosGlobalInputHook::PermissionProbeTimerCallback(CFRunLoopTimerRef timer, void* userInfo) {
    (void)timer;
    auto* self = reinterpret_cast<MacosGlobalInputHook*>(userInfo);
    if (!self) {
        return;
    }
    self->OnPermissionProbeTimer();
}
#endif

void MacosGlobalInputHook::OnPermissionProbeTimer() {
#if !defined(__APPLE__)
    return;
#else
    if (!running_.load(std::memory_order_acquire)) {
        return;
    }

    const std::string simulationFilePath = macos_input_permission::ReadPermissionSimulationFilePath();
    if (!simulationFilePath.empty()) {
        const bool trusted = macos_input_permission::IsRuntimeInputTrusted(simulationFilePath);
        lastError_.store(trusted ? kErrorSuccess : kErrorPermissionDenied, std::memory_order_release);
        return;
    }

    CFMachPortRef tap = nullptr;
    {
        std::lock_guard<std::mutex> lock(runLoopMutex_);
        tap = static_cast<CFMachPortRef>(tapRef_);
    }
    if (!tap) {
        return;
    }

    const bool trusted = AXIsProcessTrusted();
    if (!trusted) {
        lastError_.store(kErrorPermissionDenied, std::memory_order_release);
        if (CGEventTapIsEnabled(tap)) {
            CGEventTapEnable(tap, false);
        }
        return;
    }

    if (!CGEventTapIsEnabled(tap)) {
        CGEventTapEnable(tap, true);
    }
    lastError_.store(kErrorSuccess, std::memory_order_release);
#endif
}

} // namespace mousefx
