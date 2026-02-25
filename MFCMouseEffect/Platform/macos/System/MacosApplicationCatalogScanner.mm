#include "pch.h"

#include "Platform/macos/System/MacosApplicationCatalogScanner.h"

#include "Platform/macos/System/MacosApplicationCatalogScanWorkflow.h"

namespace mousefx::platform::macos {

std::vector<ApplicationCatalogEntry> MacosApplicationCatalogScanner::Scan() const {
#if !defined(__APPLE__)
    return {};
#else
    @autoreleasepool {
        return ScanMacosApplicationCatalogEntries();
    }
#endif
}

} // namespace mousefx::platform::macos
