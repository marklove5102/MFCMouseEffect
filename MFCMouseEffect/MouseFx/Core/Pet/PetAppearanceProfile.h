#pragma once

#include <string>

#include "MouseFx/Core/Pet/PetTypes.h"

namespace mousefx::pet {

struct PetAppearanceProfile final {
    AppearanceOverrides defaultAppearance{};
};

bool LoadPetAppearanceProfileFromJsonFile(const std::string& jsonPath,
                                          PetAppearanceProfile* outProfile,
                                          std::string* outError);

} // namespace mousefx::pet

