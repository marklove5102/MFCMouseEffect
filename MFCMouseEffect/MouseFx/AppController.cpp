// AppController.cpp

#include "pch.h"

#include "AppController.h"
#include "MouseFxMessages.h"

#include <new>

namespace mousefx {

static const wchar_t* kDispatchClassName = L"MouseFxDispatchWindow";
static constexpr UINT_PTR kSelfTestTimerId = 0x4D46; // 'MF' - debug-only

AppController::~AppController() {
    Stop();
}

bool AppController::Start() {
    if (dispatchHwnd_) return true;
    diag_ = {};

    diag_.stage = StartStage::GdiPlusStartup;
    if (!gdiplus_.Startup()) {
#ifdef _DEBUG
        OutputDebugStringW(L"MouseFx: GDI+ startup failed.\n");
#endif
        return false;
    }

    diag_.stage = StartStage::DispatchWindow;
    if (!CreateDispatchWindow()) {
#ifdef _DEBUG
        OutputDebugStringW(L"MouseFx: dispatch window creation failed.\n");
#endif
        Stop();
        return false;
    }

    diag_.stage = StartStage::WindowPool;
    if (!pool_.Initialize(10)) {
#ifdef _DEBUG
        OutputDebugStringW(L"MouseFx: ripple window pool init failed.\n");
#endif
        diag_.error = GetLastError();
        Stop();
        return false;
    }

    diag_.stage = StartStage::GlobalHook;
    if (!hook_.Start(dispatchHwnd_)) {
#ifdef _DEBUG
        wchar_t buf[256]{};
        wsprintfW(buf, L"MouseFx: global hook start failed. GetLastError=%lu\n", hook_.LastError());
        OutputDebugStringW(buf);
#endif
        diag_.error = hook_.LastError();
        Stop();
        return false;
    }

#ifdef _DEBUG
    // One-shot self-test: shows a ripple at the current cursor position shortly after start.
    // This helps diagnose "no ripple" issues (hook vs rendering vs wrong exe).
    SetTimer(dispatchHwnd_, kSelfTestTimerId, 250, nullptr);
#endif
    return true;
}

void AppController::Stop() {
    hook_.Stop();
    pool_.Shutdown();
    DestroyDispatchWindow();
    gdiplus_.Shutdown();
}

bool AppController::CreateDispatchWindow() {
    if (dispatchHwnd_) return true;

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = &AppController::DispatchWndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = kDispatchClassName;
    if (RegisterClassExW(&wc) == 0 && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        diag_.error = GetLastError();
        return false;
    }

    dispatchHwnd_ = CreateWindowExW(
        0,
        kDispatchClassName,
        L"",
        0,
        0, 0, 0, 0,
        HWND_MESSAGE,
        nullptr,
        GetModuleHandleW(nullptr),
        this
    );
    if (!dispatchHwnd_) {
        diag_.error = GetLastError();
    }
    return dispatchHwnd_ != nullptr;
}

void AppController::DestroyDispatchWindow() {
    if (dispatchHwnd_) {
        DestroyWindow(dispatchHwnd_);
        dispatchHwnd_ = nullptr;
    }
}

LRESULT CALLBACK AppController::DispatchWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    AppController* self = nullptr;
    if (msg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        self = reinterpret_cast<AppController*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        // `CreateWindowExW` hasn't returned yet, so `dispatchHwnd_` isn't set.
        // Ensure we never call DefWindowProc with a null/invalid HWND during creation.
        self->dispatchHwnd_ = hwnd;
    } else {
        self = reinterpret_cast<AppController*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (self) {
        return self->OnDispatchMessage(hwnd, msg, wParam, lParam);
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT AppController::OnDispatchMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_MFX_CLICK) {
        auto* ev = reinterpret_cast<ClickEvent*>(lParam);
        if (ev) {
#ifdef _DEBUG
            if (debugClickCount_ < 5) {
                debugClickCount_++;
                wchar_t buf[256]{};
                wsprintfW(buf, L"MouseFx: click received (%u) pt=(%ld,%ld) button=%u\n",
                    debugClickCount_, ev->pt.x, ev->pt.y, (unsigned)ev->button);
                OutputDebugStringW(buf);
            }
#endif
            pool_.ShowRipple(*ev);
            delete ev;
        }
        return 0;
    }
#ifdef _DEBUG
    if (msg == WM_TIMER && wParam == kSelfTestTimerId) {
        KillTimer(dispatchHwnd_, kSelfTestTimerId);
        ClickEvent ev{};
        GetCursorPos(&ev.pt);
        ev.button = MouseButton::Left;
        pool_.ShowRipple(ev);
        OutputDebugStringW(L"MouseFx: self-test ripple fired.\n");
        return 0;
    }
#endif
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

} // namespace mousefx
