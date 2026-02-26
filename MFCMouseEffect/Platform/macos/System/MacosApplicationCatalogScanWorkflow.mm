#include "pch.h"

#include "Platform/macos/System/MacosApplicationCatalogScanWorkflow.h"

#include "Platform/macos/System/MacosApplicationCatalogEntryStore.h"
#include "Platform/macos/System/MacosApplicationCatalogScanWorkflow.Internal.h"
#include "Platform/macos/System/MacosApplicationCatalogScanRoots.h"

#if defined(__APPLE__)
#endif

#include <string>
#include <unordered_map>
#include <vector>

namespace mousefx::platform::macos {

std::vector<ApplicationCatalogEntry> ScanMacosApplicationCatalogEntries() {
#if !defined(__APPLE__)
    return {};
#else
    std::vector<ApplicationCatalogEntry> entries;
    std::unordered_map<std::string, size_t> indexByProcess;

    const std::vector<MacosApplicationCatalogScanRoot> roots = BuildMacosApplicationCatalogScanRoots();
    for (const MacosApplicationCatalogScanRoot& root : roots) {
        application_catalog_scan_detail::ScanMacosApplicationCatalogRoot(root, &entries, &indexByProcess);
    }

    SortMacosApplicationCatalogEntries(&entries);
    return entries;
#endif
}

} // namespace mousefx::platform::macos
