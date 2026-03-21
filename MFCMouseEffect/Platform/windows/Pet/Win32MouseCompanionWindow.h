#pragma once

#include <windows.h>

#include <memory>
#include <string>
#include <vector>

#include "Platform/windows/Pet/Win32MouseCompanionVisualState.h"

namespace mousefx::windows {

class IWin32MouseCompanionRendererBackend;
class Win32MouseCompanionPresenter;

class Win32MouseCompanionWindow final {
public:
    Win32MouseCompanionWindow();
    ~Win32MouseCompanionWindow();

    bool Create();
    void Shutdown();
    bool Show();
    void Hide();
    bool IsVisible() const;
    bool IsCreated() const;
    bool Update(const Win32MouseCompanionVisualState& state);
    std::string PreferredRendererBackendSource() const;
    std::string PreferredRendererBackendName() const;
    std::string SelectedRendererBackendName() const;
    std::string RendererBackendSelectionReason() const;
    std::string RendererBackendFailureReason() const;
    std::vector<std::string> AvailableRendererBackendNames() const;

private:
    static constexpr UINT kMsgEnsureTopmost = WM_APP + 0x41;

    static const wchar_t* ClassName();
    static bool EnsureClassRegistered();
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static void CALLBACK ForegroundEventProc(
        HWINEVENTHOOK hook,
        DWORD event,
        HWND hwnd,
        LONG idObject,
        LONG idChild,
        DWORD idEventThread,
        DWORD eventTime);

    LRESULT OnMessage(UINT msg, WPARAM wParam, LPARAM lParam);
    void EnsureTopmostZOrder(bool force);
    bool EnsureSurface(int width, int height);
    void DestroySurface();
    bool RenderLayered(const RECT& bounds, const Win32MouseCompanionVisualState& state);

    HWND hwnd_{nullptr};
    HDC memDc_{nullptr};
    HBITMAP dib_{nullptr};
    void* bits_{nullptr};
    int surfaceWidth_{0};
    int surfaceHeight_{0};
    bool visible_{false};
    uint64_t lastTopmostEnsureMs_{0};
    HWINEVENTHOOK foregroundHook_{nullptr};
    std::unique_ptr<IWin32MouseCompanionRendererBackend> renderer_{};
    std::unique_ptr<Win32MouseCompanionPresenter> presenter_{};
    std::string preferredRendererBackendSource_;
    std::string preferredRendererBackendName_;
    std::string selectedRendererBackendName_;
    std::string rendererBackendSelectionReason_;
    std::string rendererBackendFailureReason_;
    std::vector<std::string> availableRendererBackendNames_{};
};

} // namespace mousefx::windows
