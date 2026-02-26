#pragma once

#include <string>

#if defined(__APPLE__)
#import <Foundation/Foundation.h>
#endif

namespace mousefx::platform::macos::application_catalog_scan_detail::bundle_resolve_detail {

std::string ResolveProcessName(NSURL* bundleUrl);
std::string ResolveDisplayName(NSURL* bundleUrl, const std::string& fallback);

} // namespace mousefx::platform::macos::application_catalog_scan_detail::bundle_resolve_detail
