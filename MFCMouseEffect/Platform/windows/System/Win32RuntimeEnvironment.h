#pragma once

#include "Platform/PlatformRuntimeEnvironment.h"

#include <string>

namespace mousefx::platform::windows {

RuntimeProbeResult ProbeDawnRuntimeOnce();
std::wstring GetExecutableDirectoryW();
std::wstring GetParentDirectoryW(const std::wstring& path);
std::wstring GetPreferredConfigDirectoryW();

} // namespace mousefx::platform::windows
