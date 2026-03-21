#pragma once

#include "Platform/windows/Pet/Win32MouseCompanionRendererBackendPreference.h"

namespace mousefx {
struct MouseCompanionPetRuntimeConfig;
}

namespace mousefx::windows {

Win32MouseCompanionRendererBackendPreferenceRequest BuildWin32MouseCompanionRendererBackendPreferenceRequest(
    const MouseCompanionPetRuntimeConfig& config);

} // namespace mousefx::windows
