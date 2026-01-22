#pragma once

#include <windows.h>
#include <memory>
#include <cstdint>

#include "GdiPlusSession.h"
#include "GlobalMouseHook.h"
#include "RippleWindowPool.h"

namespace mousefx {

// Owns the subsystem lifecycle: message-only dispatcher, GDI+ init, hook, and window pool.
class AppController final {
public:
    AppController() = default;
    ~AppController();

    AppController(const AppController&) = delete;
    AppController& operator=(const AppController&) = delete;

    enum class StartStage : uint8_t {
        None = 0,
        GdiPlusStartup,
        DispatchWindow,
        WindowPool,
        GlobalHook,
    };

    struct StartDiagnostics {
        StartStage stage{StartStage::None};
        DWORD error{ERROR_SUCCESS};
    };

    bool Start();
    void Stop();
    StartDiagnostics Diagnostics() const { return diag_; }

private:
    static LRESULT CALLBACK DispatchWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT OnDispatchMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    bool CreateDispatchWindow();
    void DestroyDispatchWindow();

    HWND dispatchHwnd_ = nullptr;

    GdiPlusSession gdiplus_{};
    GlobalMouseHook hook_{};
    RippleWindowPool pool_{};
    StartDiagnostics diag_{};

#ifdef _DEBUG
    uint32_t debugClickCount_ = 0;
#endif
};

} // namespace mousefx
