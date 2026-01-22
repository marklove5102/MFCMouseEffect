#pragma once

// Custom app messages used by the mouse effect subsystem.
// WM_APP range is per-process and safe for internal message passing.

#ifndef WM_APP
#define WM_APP 0x8000
#endif

namespace mousefx {

// lParam points to a heap-allocated ClickEvent (freed by the receiver).
constexpr unsigned int WM_MFX_CLICK = WM_APP + 0x210;

} // namespace mousefx

