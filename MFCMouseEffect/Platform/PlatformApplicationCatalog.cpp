#include "pch.h"

#include "Platform/PlatformApplicationCatalog.h"

#if defined(_WIN32)
#include "Platform/windows/System/Win32ApplicationCatalogScanner.h"
#elif defined(__APPLE__)
#include "Platform/macos/System/MacosApplicationCatalogScanner.h"
#endif

namespace mousefx::platform {

std::vector<ApplicationCatalogEntry> ScanApplicationCatalog() {
#if defined(_WIN32)
    windows::Win32ApplicationCatalogScanner scanner;
    return scanner.Scan();
#elif defined(__APPLE__)
    macos::MacosApplicationCatalogScanner scanner;
    return scanner.Scan();
#else
    return {};
#endif
}

} // namespace mousefx::platform
