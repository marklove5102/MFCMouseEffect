#pragma once

#include <string>
#include <string_view>

namespace mousefx {

bool IsLaunchInputValid(std::string_view url);
std::string ReadLaunchCaptureFilePath();
bool WriteLaunchCaptureFile(const std::string& filePath, const char* command, const std::string& url);
bool SpawnSettingsCommand(const char* command, const std::string& url);

} // namespace mousefx
