#pragma once

#include "Platform/windows/Pet/Win32MouseCompanionRendererBackendRegistry.h"

#include <string>
#include <vector>

namespace mousefx::windows {

std::vector<std::string> ListWin32MouseCompanionRealRendererUnmetRequirements();
Win32MouseCompanionRendererBackendRegistry::Availability
EvaluateWin32MouseCompanionRealRendererAvailability();
std::string DescribeWin32MouseCompanionRealRendererStartFailure();

} // namespace mousefx::windows
