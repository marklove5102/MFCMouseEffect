#pragma once

#include <string>

namespace mousefx::platform::windows {

bool ConfigureLaunchAtStartup(bool enabled, std::string* outError);
bool SyncLaunchAtStartupManifest(bool enabled, std::string* outError);

} // namespace mousefx::platform::windows
