#pragma once

#include "MouseFx/Core/Shell/AppShellStartOptions.h"
#include "Platform/PlatformEntryArgs.h"

namespace mousefx::platform {

// Resolves process startup options from the native platform command line.
AppShellStartOptions CreatePlatformStartupOptions();
AppShellStartOptions CreatePlatformStartupOptions(const PlatformEntryArgs& entryArgs);

} // namespace mousefx::platform
