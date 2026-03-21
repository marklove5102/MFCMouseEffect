#pragma once

#include <string>
#include <vector>

namespace mousefx::windows {

struct Win32MouseCompanionAppearanceProfile final {
    std::string skinVariantId{"default"};
    std::vector<std::string> enabledAccessoryIds;
    bool loaded{false};
};

bool LoadWin32MouseCompanionAppearanceProfileFromPath(
    const std::string& path,
    Win32MouseCompanionAppearanceProfile* outProfile);

} // namespace mousefx::windows
