#include "pch.h"

#include "ConfigPathResolver.h"

#include "Platform/PlatformRuntimeEnvironment.h"

#include <filesystem>

namespace mousefx {

std::wstring ResolveConfigDirectory() {
    // Try preferred per-user config directory first in non-debug mode.
#ifndef _DEBUG
    const std::wstring preferredDir = platform::GetPreferredConfigDirectoryW();
    if (!preferredDir.empty()) {
        std::error_code ec;
        std::filesystem::create_directories(std::filesystem::path(preferredDir), ec);
        if (!ec) {
            return preferredDir;
        }
    }
#endif

    // Fallback to EXE directory.
    const std::wstring exeDir = platform::GetExecutableDirectoryW();
    if (!exeDir.empty()) {
        return exeDir;
    }
    return L".";
}

std::wstring ResolveLocalDiagDirectory() {
    const std::wstring exeDir = platform::GetExecutableDirectoryW();
    if (exeDir.empty()) {
        return (std::filesystem::path(L".") / L".local" / L"diag").wstring();
    }
    return (std::filesystem::path(exeDir) / L".local" / L"diag").wstring();
}

} // namespace mousefx
