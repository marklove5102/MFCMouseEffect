#include "pch.h"

#include "MouseFx/Server/diagnostics/MouseCompanionRendererBackendDiagnostics.h"

#include "MouseFx/Utils/StringUtils.h"
#include "Platform/windows/Pet/Win32MouseCompanionRendererBackendPreference.h"
#include "Platform/windows/Pet/Win32MouseCompanionRendererBackendPreferenceSources.h"

namespace mousefx {
namespace {

bool StartsWithAscii(const std::string& value, const char* prefix) {
    if (!prefix) {
        return false;
    }
    return TrimAscii(value).rfind(prefix, 0) == 0;
}

std::string ResolveConfiguredPreferenceSource(const AppController::MouseCompanionRuntimeStatus& status) {
    const std::string configuredSource = TrimAscii(status.configuredRendererBackendPreferenceSource);
    return configuredSource.empty() ? kConfiguredRuntimeConfigRendererBackendPreferenceSource : configuredSource;
}

} // namespace

ConfiguredMouseCompanionRendererBackendPreferenceDiagnostics
EvaluateConfiguredMouseCompanionRendererBackendPreferenceDiagnostics(
    const AppController::MouseCompanionRuntimeStatus& status) {
    ConfiguredMouseCompanionRendererBackendPreferenceDiagnostics diagnostics{};
    const std::string configuredName = TrimAscii(status.configuredRendererBackendPreferenceName);
    if (configuredName.empty()) {
        diagnostics.status = "not_configured";
        return diagnostics;
    }

    const std::string normalizedConfiguredName =
        windows::NormalizeWin32MouseCompanionRendererBackendName(configuredName);
    const std::string normalizedPreferredName =
        windows::NormalizeWin32MouseCompanionRendererBackendName(status.preferredRendererBackend);
    const std::string configuredSource = ResolveConfiguredPreferenceSource(status);
    const std::string preferredSource = TrimAscii(status.preferredRendererBackendSource);

    if (preferredSource.empty() && TrimAscii(status.preferredRendererBackend).empty()) {
        diagnostics.status = "selection_pending";
        return diagnostics;
    }

    if (preferredSource == configuredSource && normalizedPreferredName == normalizedConfiguredName) {
        diagnostics.effective = true;
        diagnostics.status = "active";
        return diagnostics;
    }

    if (preferredSource == configuredSource) {
        diagnostics.status = "configured_name_mismatch";
        return diagnostics;
    }

    if (StartsWithAscii(preferredSource, "env:")) {
        diagnostics.status = "overridden_by_env";
        return diagnostics;
    }

    diagnostics.status = "overridden_by_other_source";
    return diagnostics;
}

} // namespace mousefx
