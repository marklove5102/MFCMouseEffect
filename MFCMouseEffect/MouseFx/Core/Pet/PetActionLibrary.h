#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "MouseFx/Core/Pet/PetTypes.h"

namespace mousefx::pet {

enum class ClipInterpolation : uint8_t {
    Step = 0,
    Linear = 1,
};

struct ActionBoneKeyframe final {
    float timeSeconds{0.0f};
    Transform local{};
    ClipInterpolation interpolation{ClipInterpolation::Linear};
};

struct ActionBoneTrack final {
    std::string boneName{};
    std::vector<ActionBoneKeyframe> keyframes{};
};

struct ActionClip final {
    PetAction action{PetAction::Idle};
    float durationSeconds{0.0f};
    bool loop{true};
    std::vector<ActionBoneTrack> tracks{};
};

struct ActionBoneRemapRule final {
    std::string sourceBone{};
    std::vector<std::string> candidateBones{};
};

struct ActionLibrary final {
    std::vector<ActionClip> clips{};
    std::vector<ActionBoneRemapRule> boneRemaps{};
    const ActionClip* FindClip(PetAction action) const;
    const ActionBoneRemapRule* FindBoneRemap(const std::string& sourceBone) const;
};

bool LoadActionLibraryFromJsonFile(const std::string& jsonPath,
                                   ActionLibrary* outLibrary,
                                   std::string* outError);

} // namespace mousefx::pet
