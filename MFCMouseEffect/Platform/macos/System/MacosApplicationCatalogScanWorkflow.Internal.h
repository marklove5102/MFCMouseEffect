#pragma once

#include "Platform/PlatformApplicationCatalog.h"
#include "Platform/macos/System/MacosApplicationCatalogScanRoots.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace mousefx::platform::macos::application_catalog_scan_detail {

void ScanMacosApplicationCatalogRoot(
    const MacosApplicationCatalogScanRoot& root,
    std::vector<ApplicationCatalogEntry>* entries,
    std::unordered_map<std::string, size_t>* indexByProcess);

} // namespace mousefx::platform::macos::application_catalog_scan_detail
