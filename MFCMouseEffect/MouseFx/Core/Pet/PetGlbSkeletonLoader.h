#pragma once

#include <string>

#include "MouseFx/Core/Pet/PetTypes.h"

namespace mousefx::pet {

bool LoadSkeletonFromGlb(const std::string& glbPath, SkeletonDesc* outSkeleton, std::string* outError);

} // namespace mousefx::pet
