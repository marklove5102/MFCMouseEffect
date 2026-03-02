#pragma once

#include "Platform/PlatformApplicationCatalog.h"

#include <vector>

namespace mousefx::platform::macos {

// Scans local macOS app bundles and resolves automation process names.
class MacosApplicationCatalogScanner final {
public:
    std::vector<ApplicationCatalogEntry> Scan() const;
};

} // namespace mousefx::platform::macos
