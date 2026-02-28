#pragma once

#include <cstdint>
#include <string>

#include "MouseFx/Core/Protocol/InputTypes.h"

namespace mousefx::diagnostics {

struct TextEffectRuntimeSnapshot {
    uint64_t clickCount = 0;
    uint64_t fallbackShowCount = 0;
    uint64_t fallbackPanelCreated = 0;
    uint64_t fallbackErrorCount = 0;
    uint64_t lastClickMs = 0;
    uint64_t lastFallbackMs = 0;
    uint64_t fallbackActivePanels = 0;
    ScreenPoint lastClickPt{};
    ScreenPoint lastFallbackPt{};
    std::string lastTextPreview{};
    std::string lastError{};
};

void RecordTextEffectClick(const ScreenPoint& pt, const std::wstring& text);
void RecordTextEffectFallbackShow(const ScreenPoint& pt, const std::wstring& text);
void RecordTextEffectFallbackPanelCreated();
void RecordTextEffectFallbackError(const char* reason);
void SetTextEffectFallbackActivePanels(uint64_t count);
TextEffectRuntimeSnapshot GetTextEffectRuntimeSnapshot();

} // namespace mousefx::diagnostics
