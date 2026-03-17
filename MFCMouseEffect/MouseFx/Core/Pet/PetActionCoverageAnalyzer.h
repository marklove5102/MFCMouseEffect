#pragma once

#include <string>
#include <vector>

#include "MouseFx/Core/Pet/PetActionLibrary.h"
#include "MouseFx/Core/Pet/PetTypes.h"

namespace mousefx::pet {

struct ActionCoverageEntry final {
    PetAction action{PetAction::Idle};
    std::string actionName{};
    bool clipPresent{false};
    int trackCount{0};
    int mappedTrackCount{0};
    float coverageRatio{0.0f};
    std::vector<std::string> missingBoneTracks{};
};

struct ActionCoverageReport final {
    int expectedActionCount{0};
    int coveredActionCount{0};
    int missingActionCount{0};
    int skeletonBoneCount{0};
    int totalTrackCount{0};
    int totalMappedTrackCount{0};
    float overallCoverageRatio{0.0f};
    std::vector<std::string> missingActions{};
    std::vector<std::string> missingBoneNames{};
    std::vector<ActionCoverageEntry> actions{};
};

const char* PetActionName(PetAction action);

ActionCoverageReport BuildActionCoverageReport(
    const SkeletonDesc& skeleton,
    const ActionLibrary& library);

} // namespace mousefx::pet
