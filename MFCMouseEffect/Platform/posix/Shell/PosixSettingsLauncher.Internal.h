#pragma once

#include <string>
#include <string_view>

namespace mousefx {

bool IsLaunchInputValid(std::string_view value);
std::string ReadLaunchCaptureFilePath();
bool WriteLaunchCaptureFile(const std::string& filePath, const char* command, const std::string& url);
bool WriteLaunchCaptureFileForApp(const std::string& filePath, const char* command, const std::string& appPath);
bool SpawnSettingsCommand(const char* command, const std::string& target);

} // namespace mousefx
