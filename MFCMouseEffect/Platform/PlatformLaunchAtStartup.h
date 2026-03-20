#pragma once

#include <string>

namespace mousefx::platform {

// Returns whether native launch-at-startup control is implemented on current platform.
bool IsLaunchAtStartupSupported();

// Applies launch-at-startup policy to the native platform.
// Best-effort API: returns true on successful native apply.
bool ConfigureLaunchAtStartup(bool enabled, std::string* outError);

// Reconciles the persisted launch-at-startup manifest without touching the
// currently loaded runtime service. Use this during normal app startup so the
// manifest stays in sync without spawning/killing duplicate instances.
bool SyncLaunchAtStartupManifest(bool enabled, std::string* outError);

} // namespace mousefx::platform
