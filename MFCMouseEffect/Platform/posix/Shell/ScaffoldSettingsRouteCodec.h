#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace mousefx::platform::scaffold {

bool IsSettingsUrlShellSafe(std::string_view url);
bool TryParseUnsignedPort(std::string_view text, uint16_t* outPort);
std::string UrlDecodePercentCopy(std::string_view value);
std::string UrlEncodeQueryValue(std::string_view value);

} // namespace mousefx::platform::scaffold
