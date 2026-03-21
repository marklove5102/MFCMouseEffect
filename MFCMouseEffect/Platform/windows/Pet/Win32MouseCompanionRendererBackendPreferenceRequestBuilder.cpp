#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionRendererBackendPreferenceRequestBuilder.h"
#include "Platform/windows/Pet/Win32MouseCompanionRendererBackendPreferenceSources.h"

#include "MouseFx/Core/Control/MouseCompanionPluginV1Types.h"
#include "MouseFx/Utils/StringUtils.h"

namespace mousefx::windows {

Win32MouseCompanionRendererBackendPreferenceRequest BuildWin32MouseCompanionRendererBackendPreferenceRequest(
    const MouseCompanionPetRuntimeConfig& config) {
    Win32MouseCompanionRendererBackendPreferenceRequest request{};
    request.preferredBackendSource = config.rendererBackendPreferenceSource;
    request.preferredBackendName = config.rendererBackendPreferenceName;
    if (TrimAscii(request.preferredBackendSource).empty() && !TrimAscii(request.preferredBackendName).empty()) {
        request.preferredBackendSource = kConfiguredRuntimeConfigRendererBackendPreferenceSource;
    }
    return request;
}

} // namespace mousefx::windows
