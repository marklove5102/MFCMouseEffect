#include "pch.h"

#include "Platform/macos/System/MacosApplicationCatalogScanWorkflow.BundleResolve.Internal.h"
#include "Platform/macos/System/MacosApplicationCatalogScanWorkflow.Internal.h"

#if defined(__APPLE__)
#import <Foundation/Foundation.h>
#endif

#include <string>

namespace mousefx::platform::macos::application_catalog_scan_detail {
bool ResolveMacosApplicationCatalogEntryFromPath(
    const std::string& bundlePath,
    std::string* processName,
    std::string* displayName) {
#if !defined(__APPLE__)
    (void)bundlePath;
    (void)processName;
    (void)displayName;
    return false;
#else
    if (!processName || !displayName || bundlePath.empty()) {
        return false;
    }

    NSString* bundlePathNs = [NSString stringWithUTF8String:bundlePath.c_str()];
    if (bundlePathNs == nil || bundlePathNs.length == 0) {
        return false;
    }

    NSURL* bundleUrl = [NSURL fileURLWithPath:bundlePathNs isDirectory:YES];
    if (bundleUrl == nil) {
        return false;
    }

    const std::string resolvedProcess = bundle_resolve_detail::ResolveProcessName(bundleUrl);
    if (resolvedProcess.empty()) {
        return false;
    }
    *processName = resolvedProcess;
    *displayName = bundle_resolve_detail::ResolveDisplayName(bundleUrl, resolvedProcess);
    return true;
#endif
}

} // namespace mousefx::platform::macos::application_catalog_scan_detail
