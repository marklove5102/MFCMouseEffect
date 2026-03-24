#pragma once

#include <string>
#include <vector>

namespace mousefx::windows {

std::string NormalizeWin32MouseCompanionNodeMatchToken(const std::string& value);
std::vector<std::string> TokenizeWin32MouseCompanionNodeMatchText(const std::string& value);

} // namespace mousefx::windows
