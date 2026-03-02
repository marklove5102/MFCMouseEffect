#include "Platform/macos/Shell/MacosDpiAwarenessService.h"

namespace mousefx {

void MacosDpiAwarenessService::EnableForScreenCoords() {
    // macOS apps are DPI-aware by default; no bootstrap is required.
}

} // namespace mousefx
