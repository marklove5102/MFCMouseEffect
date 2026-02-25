#pragma once

#include "Platform/PlatformApplicationCatalog.h"

#include <vector>

namespace mousefx::platform::macos {

std::vector<ApplicationCatalogEntry> ScanMacosApplicationCatalogEntries();

} // namespace mousefx::platform::macos
