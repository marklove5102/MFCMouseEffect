#pragma once

#include <string>

namespace mousefx::windows {

struct Win32MouseCompanionRendererBackendPreference {
    std::string source;
    std::string backendName;
};

struct Win32MouseCompanionRendererBackendPreferenceRequest {
    std::string preferredBackendSource;
    std::string preferredBackendName;
};

std::string NormalizeWin32MouseCompanionRendererBackendName(const std::string& rawBackendName);
bool IsAutoWin32MouseCompanionRendererBackendName(const std::string& backendName);
void RegisterBuiltInWin32MouseCompanionRendererBackendPreferenceSources();
Win32MouseCompanionRendererBackendPreference ResolveWin32MouseCompanionRendererBackendPreference(
    const Win32MouseCompanionRendererBackendPreferenceRequest& request);
Win32MouseCompanionRendererBackendPreference ResolveWin32MouseCompanionRendererBackendPreference();
Win32MouseCompanionRendererBackendPreference ResolveExplicitWin32MouseCompanionRendererBackendPreference(
    const std::string& preferredBackendName);

} // namespace mousefx::windows
