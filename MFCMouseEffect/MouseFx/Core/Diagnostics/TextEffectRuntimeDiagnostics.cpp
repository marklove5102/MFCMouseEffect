#include "pch.h"

#include "MouseFx/Core/Diagnostics/TextEffectRuntimeDiagnostics.h"

#include "MouseFx/Utils/StringUtils.h"
#include "MouseFx/Utils/TimeUtils.h"

#include <algorithm>
#include <atomic>
#include <mutex>

namespace mousefx::diagnostics {
namespace {

constexpr size_t kTextPreviewLimit = 64;

struct TextEffectDiagState final {
    std::atomic<uint64_t> clickCount{0};
    std::atomic<uint64_t> fallbackShowCount{0};
    std::atomic<uint64_t> fallbackPanelCreated{0};
    std::atomic<uint64_t> fallbackErrorCount{0};
    std::atomic<uint64_t> lastClickMs{0};
    std::atomic<uint64_t> lastFallbackMs{0};
    std::atomic<uint64_t> fallbackActivePanels{0};
    std::atomic<int32_t> lastClickX{0};
    std::atomic<int32_t> lastClickY{0};
    std::atomic<int32_t> lastFallbackX{0};
    std::atomic<int32_t> lastFallbackY{0};
    std::mutex textMutex{};
    std::string lastTextPreview{};
    std::string lastError{};
};

TextEffectDiagState& State() {
    static TextEffectDiagState state{};
    return state;
}

std::string BuildPreview(const std::wstring& text) {
    std::string utf8 = Utf16ToUtf8(text.c_str());
    if (utf8.size() <= kTextPreviewLimit) {
        return utf8;
    }
    utf8.resize(kTextPreviewLimit);
    return utf8;
}

void StorePreview(const std::wstring& text) {
    auto& state = State();
    std::lock_guard<std::mutex> lock(state.textMutex);
    state.lastTextPreview = BuildPreview(text);
}

void StoreError(const char* reason) {
    if (!reason) {
        return;
    }
    auto& state = State();
    std::lock_guard<std::mutex> lock(state.textMutex);
    state.lastError = reason;
}

} // namespace

void RecordTextEffectClick(const ScreenPoint& pt, const std::wstring& text) {
    auto& state = State();
    state.clickCount.fetch_add(1, std::memory_order_relaxed);
    state.lastClickMs.store(NowMs(), std::memory_order_relaxed);
    state.lastClickX.store(pt.x, std::memory_order_relaxed);
    state.lastClickY.store(pt.y, std::memory_order_relaxed);
    if (!text.empty()) {
        StorePreview(text);
    }
}

void RecordTextEffectFallbackShow(const ScreenPoint& pt, const std::wstring& text) {
    auto& state = State();
    state.fallbackShowCount.fetch_add(1, std::memory_order_relaxed);
    state.lastFallbackMs.store(NowMs(), std::memory_order_relaxed);
    state.lastFallbackX.store(pt.x, std::memory_order_relaxed);
    state.lastFallbackY.store(pt.y, std::memory_order_relaxed);
    if (!text.empty()) {
        StorePreview(text);
    }
}

void RecordTextEffectFallbackPanelCreated() {
    State().fallbackPanelCreated.fetch_add(1, std::memory_order_relaxed);
}

void RecordTextEffectFallbackError(const char* reason) {
    auto& state = State();
    state.fallbackErrorCount.fetch_add(1, std::memory_order_relaxed);
    StoreError(reason);
}

void SetTextEffectFallbackActivePanels(uint64_t count) {
    State().fallbackActivePanels.store(count, std::memory_order_relaxed);
}

TextEffectRuntimeSnapshot GetTextEffectRuntimeSnapshot() {
    auto& state = State();
    TextEffectRuntimeSnapshot out{};
    out.clickCount = state.clickCount.load(std::memory_order_relaxed);
    out.fallbackShowCount = state.fallbackShowCount.load(std::memory_order_relaxed);
    out.fallbackPanelCreated = state.fallbackPanelCreated.load(std::memory_order_relaxed);
    out.fallbackErrorCount = state.fallbackErrorCount.load(std::memory_order_relaxed);
    out.lastClickMs = state.lastClickMs.load(std::memory_order_relaxed);
    out.lastFallbackMs = state.lastFallbackMs.load(std::memory_order_relaxed);
    out.fallbackActivePanels = state.fallbackActivePanels.load(std::memory_order_relaxed);
    out.lastClickPt.x = state.lastClickX.load(std::memory_order_relaxed);
    out.lastClickPt.y = state.lastClickY.load(std::memory_order_relaxed);
    out.lastFallbackPt.x = state.lastFallbackX.load(std::memory_order_relaxed);
    out.lastFallbackPt.y = state.lastFallbackY.load(std::memory_order_relaxed);
    {
        std::lock_guard<std::mutex> lock(state.textMutex);
        out.lastTextPreview = state.lastTextPreview;
        out.lastError = state.lastError;
    }
    return out;
}

} // namespace mousefx::diagnostics
