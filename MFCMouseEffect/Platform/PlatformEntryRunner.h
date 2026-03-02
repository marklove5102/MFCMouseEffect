#pragma once

#include "Platform/PlatformEntryArgs.h"

namespace mousefx::platform {

// Shared process entry runner used by platform-specific entry points.
int RunPlatformEntry();
int RunPlatformEntry(const PlatformEntryArgs& entryArgs);

} // namespace mousefx::platform
