#pragma once

#include <string>

namespace mousefx {

// Shared POSIX launcher for opening settings URLs with platform shell tools.
bool LaunchUrlWithPosixCommand(const char* command, const std::string& url);

} // namespace mousefx
