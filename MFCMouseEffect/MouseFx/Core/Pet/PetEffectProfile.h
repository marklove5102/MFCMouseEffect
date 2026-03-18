#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "MouseFx/Core/Pet/PetTypes.h"

namespace mousefx::pet {

struct ProceduralActionSpec final {
    float actionIntensity = 0.6f;
    // When clip exists: final = lerp(procedural, clip, clipBlendWeight).
    // 1.0 => clip only, 0.0 => procedural only.
    float clipBlendWeight = 0.35f;
    float breatheHz = 2.4f;
    float breatheAmplitude = 0.04f;
    float pulseDecayHz = 8.0f;
    float pulseGain = 0.0f;

    float hipsYOffset = 0.0f;
    float hipsPitch = 0.0f;
    float hipsYaw = 0.0f;
    float hipsScaleX = 1.0f;
    float hipsScaleY = 1.0f;
    float hipsScaleZ = 1.0f;

    float spinePitch = 0.0f;
    float spineYaw = 0.0f;
    float chestPitch = 0.0f;
    float chestYaw = 0.0f;
    float neckPitch = 0.0f;
    float neckYaw = 0.0f;

    float headCursorPitchGain = 0.4f;
    float headPitchBias = 0.0f;
    float headYawGain = 0.45f;
    float headYawBias = 0.0f;
};

struct ProceduralEffectProfile final {
    int version = 1;
    std::unordered_map<int, ProceduralActionSpec> actions{};

    const ProceduralActionSpec* Find(PetAction action) const;
};

// Load a JSON profile from disk.
// Returns false when file is missing/invalid; error is optional.
bool LoadProceduralEffectProfileFromJsonFile(
    const std::string& jsonPath,
    ProceduralEffectProfile* outProfile,
    std::string* outError);

// Built-in profile used when no external profile is available.
std::shared_ptr<const ProceduralEffectProfile> CreateDefaultProceduralEffectProfile();

} // namespace mousefx::pet
