#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionWindow.h"

#include "Platform/windows/Pet/IWin32MouseCompanionRendererBackend.h"
#include "Platform/windows/Pet/Win32MouseCompanionPresenter.h"
#include "Platform/windows/Pet/Win32MouseCompanionRendererBackendFactory.h"
#include "Platform/windows/Pet/Win32MouseCompanionRendererInputBuilder.h"
#include "MouseFx/Utils/StringUtils.h"
#include "MouseFx/Utils/TimeUtils.h"

#include <algorithm>

#pragma comment(lib, "gdiplus.lib")

namespace mousefx::windows {
namespace {

constexpr uint64_t kTopmostReassertIntervalMs = 2500;
Win32MouseCompanionWindow* g_foregroundHookOwner = nullptr;

bool RendererBackendPreferenceRequestsEqual(
    const Win32MouseCompanionRendererBackendPreferenceRequest& lhs,
    const Win32MouseCompanionRendererBackendPreferenceRequest& rhs) {
    return TrimAscii(lhs.preferredBackendSource) == TrimAscii(rhs.preferredBackendSource) &&
           NormalizeWin32MouseCompanionRendererBackendName(lhs.preferredBackendName) ==
               NormalizeWin32MouseCompanionRendererBackendName(rhs.preferredBackendName);
}

} // namespace

Win32MouseCompanionWindow::Win32MouseCompanionWindow()
    : presenter_(std::make_unique<Win32MouseCompanionPresenter>()) {}

Win32MouseCompanionWindow::~Win32MouseCompanionWindow() {
    Shutdown();
}

const wchar_t* Win32MouseCompanionWindow::ClassName() {
    return L"MouseFxWin32MouseCompanionWindow";
}

void Win32MouseCompanionWindow::SetRendererBackendPreferenceRequest(
    const Win32MouseCompanionRendererBackendPreferenceRequest& request) {
    if (RendererBackendPreferenceRequestsEqual(rendererBackendPreferenceRequest_, request)) {
        return;
    }
    rendererBackendPreferenceRequest_ = request;
    if (!renderer_) {
        return;
    }
    ApplyRendererBackendSelection(rendererBackendPreferenceRequest_, true);
}

bool Win32MouseCompanionWindow::EnsureClassRegistered() {
    static bool registered = false;
    static bool ok = false;
    if (registered) {
        return ok;
    }
    registered = true;

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = &Win32MouseCompanionWindow::WndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = ClassName();
    ok = (RegisterClassExW(&wc) != 0) || (GetLastError() == ERROR_CLASS_ALREADY_EXISTS);
    return ok;
}

bool Win32MouseCompanionWindow::Create() {
    if (hwnd_) {
        return true;
    }
    if (!EnsureRendererBackendSelected()) {
        return false;
    }
    if (!EnsureClassRegistered()) {
        return false;
    }
    const DWORD ex = WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_NOACTIVATE;
    hwnd_ = CreateWindowExW(
        ex,
        ClassName(),
        L"",
        WS_POPUP,
        0,
        0,
        1,
        1,
        nullptr,
        nullptr,
        GetModuleHandleW(nullptr),
        this);
    if (!hwnd_) {
        return false;
    }

    foregroundHook_ = SetWinEventHook(
        EVENT_SYSTEM_FOREGROUND,
        EVENT_SYSTEM_FOREGROUND,
        nullptr,
        &Win32MouseCompanionWindow::ForegroundEventProc,
        0,
        0,
        WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);
    if (foregroundHook_) {
        g_foregroundHookOwner = this;
    }
    return true;
}

bool Win32MouseCompanionWindow::EnsureRendererBackendSelected() {
    if (renderer_) {
        return true;
    }
    return ApplyRendererBackendSelection(rendererBackendPreferenceRequest_, false);
}

bool Win32MouseCompanionWindow::ApplyRendererBackendSelection(
    const Win32MouseCompanionRendererBackendPreferenceRequest& request,
    bool preserveCurrentOnFailure) {
    auto selection = SelectDefaultWin32MouseCompanionRendererBackend(request);
    if (!selection.backend && preserveCurrentOnFailure && renderer_) {
        preferredRendererBackendSource_ = std::move(selection.preferredBackendSource);
        preferredRendererBackendName_ = std::move(selection.preferredBackendName);
        rendererBackendSelectionReason_ = std::move(selection.selectionReason);
        rendererBackendFailureReason_ = std::move(selection.failureReason);
        availableRendererBackendNames_ = std::move(selection.availableBackendNames);
        unavailableRendererBackendNames_ = std::move(selection.unavailableBackendReasons);
        return false;
    }

    if (renderer_) {
        renderer_->Shutdown();
    }
    renderer_ = std::move(selection.backend);
    preferredRendererBackendSource_ = std::move(selection.preferredBackendSource);
    preferredRendererBackendName_ = std::move(selection.preferredBackendName);
    selectedRendererBackendName_ = std::move(selection.selectedBackendName);
    rendererBackendSelectionReason_ = std::move(selection.selectionReason);
    rendererBackendFailureReason_ = std::move(selection.failureReason);
    availableRendererBackendNames_ = std::move(selection.availableBackendNames);
    unavailableRendererBackendNames_ = std::move(selection.unavailableBackendReasons);
    if (!renderer_) {
        selectedRendererBackendName_.clear();
    }
    return renderer_ != nullptr;
}

void Win32MouseCompanionWindow::ClearRendererBackendSelectionDiagnostics() {
    preferredRendererBackendSource_.clear();
    preferredRendererBackendName_.clear();
    selectedRendererBackendName_.clear();
    rendererBackendSelectionReason_.clear();
    rendererBackendFailureReason_.clear();
    availableRendererBackendNames_.clear();
    unavailableRendererBackendNames_.clear();
}

void Win32MouseCompanionWindow::Shutdown() {
    if (foregroundHook_) {
        UnhookWinEvent(foregroundHook_);
        foregroundHook_ = nullptr;
    }
    if (g_foregroundHookOwner == this) {
        g_foregroundHookOwner = nullptr;
    }
    if (hwnd_) {
        DestroyWindow(hwnd_);
        hwnd_ = nullptr;
    }
    if (renderer_) {
        renderer_->Shutdown();
    }
    renderer_.reset();
    ClearRendererBackendSelectionDiagnostics();
    DestroySurface();
    visible_ = false;
}

bool Win32MouseCompanionWindow::Show() {
    if (!hwnd_) {
        return false;
    }
    ShowWindow(hwnd_, SW_SHOWNA);
    visible_ = true;
    EnsureTopmostZOrder(true);
    return true;
}

void Win32MouseCompanionWindow::Hide() {
    if (hwnd_) {
        ShowWindow(hwnd_, SW_HIDE);
    }
    visible_ = false;
}

bool Win32MouseCompanionWindow::IsVisible() const {
    return visible_;
}

bool Win32MouseCompanionWindow::IsCreated() const {
    return hwnd_ != nullptr;
}

bool Win32MouseCompanionWindow::Update(const Win32MouseCompanionVisualState& state) {
    if (!hwnd_ || !presenter_) {
        return false;
    }
    const RECT bounds = presenter_->ResolveWindowBounds(state);
    const int width = (std::max)(1, static_cast<int>(bounds.right - bounds.left));
    const int height = (std::max)(1, static_cast<int>(bounds.bottom - bounds.top));
    if (!EnsureSurface(width, height)) {
        return false;
    }
    return RenderLayered(bounds, state);
}

std::string Win32MouseCompanionWindow::SelectedRendererBackendName() const {
    return selectedRendererBackendName_;
}

std::string Win32MouseCompanionWindow::PreferredRendererBackendName() const {
    return preferredRendererBackendName_;
}

std::string Win32MouseCompanionWindow::PreferredRendererBackendSource() const {
    return preferredRendererBackendSource_;
}

std::string Win32MouseCompanionWindow::RendererBackendSelectionReason() const {
    return rendererBackendSelectionReason_;
}

std::string Win32MouseCompanionWindow::RendererBackendFailureReason() const {
    return rendererBackendFailureReason_;
}

std::vector<std::string> Win32MouseCompanionWindow::AvailableRendererBackendNames() const {
    return availableRendererBackendNames_;
}

std::vector<std::string> Win32MouseCompanionWindow::UnavailableRendererBackendNames() const {
    return unavailableRendererBackendNames_;
}

LRESULT CALLBACK Win32MouseCompanionWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    Win32MouseCompanionWindow* self = nullptr;
    if (msg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        self = reinterpret_cast<Win32MouseCompanionWindow*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        if (self) {
            self->hwnd_ = hwnd;
        }
    } else {
        self = reinterpret_cast<Win32MouseCompanionWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }
    return self ? self->OnMessage(msg, wParam, lParam) : DefWindowProcW(hwnd, msg, wParam, lParam);
}

void CALLBACK Win32MouseCompanionWindow::ForegroundEventProc(
    HWINEVENTHOOK,
    DWORD event,
    HWND hwnd,
    LONG,
    LONG,
    DWORD,
    DWORD) {
    if (event != EVENT_SYSTEM_FOREGROUND) {
        return;
    }
    Win32MouseCompanionWindow* self = g_foregroundHookOwner;
    if (!self || !self->hwnd_ || hwnd == self->hwnd_) {
        return;
    }
    PostMessageW(self->hwnd_, kMsgEnsureTopmost, 0, 0);
}

LRESULT Win32MouseCompanionWindow::OnMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_NCHITTEST:
        return HTTRANSPARENT;
    case WM_DESTROY:
        visible_ = false;
        break;
    case kMsgEnsureTopmost:
        EnsureTopmostZOrder(true);
        return 0;
    default:
        break;
    }
    return DefWindowProcW(hwnd_, msg, wParam, lParam);
}

void Win32MouseCompanionWindow::EnsureTopmostZOrder(bool force) {
    if (!hwnd_) {
        return;
    }
    const uint64_t now = NowMs();
    if (!force && (now - lastTopmostEnsureMs_) < kTopmostReassertIntervalMs) {
        return;
    }
    lastTopmostEnsureMs_ = now;
    SetWindowPos(
        hwnd_,
        HWND_TOPMOST,
        0,
        0,
        0,
        0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
}

bool Win32MouseCompanionWindow::EnsureSurface(int width, int height) {
    if (surfaceWidth_ == width && surfaceHeight_ == height && memDc_ && bits_) {
        return true;
    }
    DestroySurface();

    HDC screen = GetDC(nullptr);
    memDc_ = CreateCompatibleDC(screen);

    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    dib_ = CreateDIBSection(screen, &bmi, DIB_RGB_COLORS, &bits_, nullptr, 0);
    if (dib_) {
        SelectObject(memDc_, dib_);
    }
    ReleaseDC(nullptr, screen);

    if (!memDc_ || !dib_ || !bits_) {
        DestroySurface();
        return false;
    }
    surfaceWidth_ = width;
    surfaceHeight_ = height;
    return true;
}

void Win32MouseCompanionWindow::DestroySurface() {
    if (dib_) {
        DeleteObject(dib_);
        dib_ = nullptr;
    }
    if (memDc_) {
        DeleteDC(memDc_);
        memDc_ = nullptr;
    }
    bits_ = nullptr;
    surfaceWidth_ = 0;
    surfaceHeight_ = 0;
}

bool Win32MouseCompanionWindow::RenderLayered(const RECT& bounds, const Win32MouseCompanionVisualState& state) {
    if (!hwnd_ || !memDc_ || !bits_ || !renderer_ || !renderer_->IsReady()) {
        return false;
    }
    ZeroMemory(bits_, static_cast<size_t>(surfaceWidth_) * static_cast<size_t>(surfaceHeight_) * 4);

    Gdiplus::Bitmap bitmap(
        surfaceWidth_,
        surfaceHeight_,
        surfaceWidth_ * 4,
        PixelFormat32bppPARGB,
        static_cast<BYTE*>(bits_));
    Gdiplus::Graphics graphics(&bitmap);
    const Win32MouseCompanionRendererInput input = BuildWin32MouseCompanionRendererInput(state);
    renderer_->Render(input, &graphics, surfaceWidth_, surfaceHeight_);

    POINT src{0, 0};
    POINT dst{bounds.left, bounds.top};
    SIZE size{surfaceWidth_, surfaceHeight_};
    BLENDFUNCTION blend{};
    blend.BlendOp = AC_SRC_OVER;
    blend.SourceConstantAlpha = 255;
    blend.AlphaFormat = AC_SRC_ALPHA;
    UpdateLayeredWindow(hwnd_, nullptr, &dst, &size, memDc_, &src, 0, &blend, ULW_ALPHA);
    EnsureTopmostZOrder(false);
    return true;
}

} // namespace mousefx::windows
