#include "pch.h"

#include "Platform/macos/Overlay/MacosInputIndicatorOverlayInternals.h"

#include "MouseFx/Core/Overlay/InputIndicatorLabelFormatter.h"

#include <algorithm>
#if defined(__APPLE__)
#include <pthread.h>
#endif

namespace mousefx::macos_input_indicator {

int ClampInt(int value, int lo, int hi) {
    return std::max(lo, std::min(value, hi));
}

std::string MouseButtonLabel(MouseButton button) {
    switch (button) {
    case MouseButton::Left:
        return "L";
    case MouseButton::Right:
        return "R";
    case MouseButton::Middle:
        return "M";
    default:
        return "?";
    }
}

std::string ScrollLabel(int delta) {
    return delta >= 0 ? "W+" : "W-";
}

std::string KeyLabel(const KeyEvent& ev) {
    InputIndicatorKeyLabelOptions options{};
    options.metaModifierLabel = "Cmd";
    return BuildInputIndicatorKeyLabel(ev, options);
}

#if defined(__APPLE__)
void RunOnMainThreadSync(dispatch_block_t block) {
    if (!block) {
        return;
    }
    if (pthread_main_np() != 0) {
        block();
        return;
    }
    dispatch_sync(dispatch_get_main_queue(), block);
}

void RunOnMainThreadAsync(dispatch_block_t block) {
    if (!block) {
        return;
    }
    dispatch_async(dispatch_get_main_queue(), block);
}

void FlushMainThreadQueueSync() {
    if (pthread_main_np() != 0) {
        return;
    }
    dispatch_sync(dispatch_get_main_queue(), ^{});
}
#endif

} // namespace mousefx::macos_input_indicator
