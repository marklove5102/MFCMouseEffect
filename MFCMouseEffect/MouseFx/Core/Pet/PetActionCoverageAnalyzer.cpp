#include "pch.h"
#include "MouseFx/Core/Pet/PetActionCoverageAnalyzer.h"

#include "MouseFx/Utils/StringUtils.h"

#include <array>
#include <algorithm>

namespace mousefx::pet {
namespace {

std::string NormalizeToken(const std::string& value) {
    return ToLowerAscii(value);
}

bool ContainsToken(const std::string& text, const std::string& token) {
    if (text.empty() || token.empty()) {
        return false;
    }
    return text.find(token) != std::string::npos || token.find(text) != std::string::npos;
}

std::vector<std::string> ResolveBoneAliases(const std::string& normalizedBoneName) {
    if (normalizedBoneName == "chest" ||
        normalizedBoneName == "upperchest" ||
        normalizedBoneName == "spine2" ||
        normalizedBoneName == "spine_03" ||
        normalizedBoneName == "spine03") {
        return {"chest", "upperchest", "spine2", "spine_03", "spine03"};
    }
    if (normalizedBoneName == "spine" ||
        normalizedBoneName == "spine1" ||
        normalizedBoneName == "spine_01" ||
        normalizedBoneName == "spine01") {
        return {"spine", "spine1", "spine_01", "spine01"};
    }
    return {};
}

int ResolveTrackBoneIndexByNormalizedName(const SkeletonDesc& skeleton, const std::string& normalizedTrack) {
    if (normalizedTrack.empty()) {
        return -1;
    }
    for (size_t i = 0; i < skeleton.bones.size(); ++i) {
        if (NormalizeToken(skeleton.bones[i].name) == normalizedTrack) {
            return static_cast<int>(i);
        }
    }

    for (size_t i = 0; i < skeleton.bones.size(); ++i) {
        const std::string normalizedBone = NormalizeToken(skeleton.bones[i].name);
        if (ContainsToken(normalizedBone, normalizedTrack) ||
            ContainsToken(normalizedTrack, normalizedBone)) {
            return static_cast<int>(i);
        }
    }

    const std::vector<std::string> aliases = ResolveBoneAliases(normalizedTrack);
    for (const auto& alias : aliases) {
        for (size_t i = 0; i < skeleton.bones.size(); ++i) {
            const std::string normalizedBone = NormalizeToken(skeleton.bones[i].name);
            if (ContainsToken(normalizedBone, alias)) {
                return static_cast<int>(i);
            }
        }
    }

    return -1;
}

int ResolveTrackBoneIndex(const SkeletonDesc& skeleton,
                          const ActionLibrary& library,
                          const std::string& trackBoneName) {
    if (trackBoneName.empty()) {
        return -1;
    }

    const std::string normalizedTrack = NormalizeToken(trackBoneName);
    const int direct = ResolveTrackBoneIndexByNormalizedName(skeleton, normalizedTrack);
    if (direct >= 0) {
        return direct;
    }

    const ActionBoneRemapRule* remap = library.FindBoneRemap(trackBoneName);
    if (!remap) {
        return -1;
    }
    for (const auto& candidateBone : remap->candidateBones) {
        const int remapped =
            ResolveTrackBoneIndexByNormalizedName(skeleton, NormalizeToken(candidateBone));
        if (remapped >= 0) {
            return remapped;
        }
    }
    return -1;
}

void AppendUnique(std::vector<std::string>* values, const std::string& item) {
    if (!values || item.empty()) {
        return;
    }
    for (const auto& existing : *values) {
        if (existing == item) {
            return;
        }
    }
    values->push_back(item);
}

} // namespace

const char* PetActionName(PetAction action) {
    switch (action) {
    case PetAction::Idle:
        return "idle";
    case PetAction::Follow:
        return "follow";
    case PetAction::ClickReact:
        return "click_react";
    case PetAction::Drag:
        return "drag";
    case PetAction::HoverReact:
        return "hover_react";
    case PetAction::HoldReact:
        return "hold_react";
    case PetAction::ScrollReact:
        return "scroll_react";
    default:
        return "unknown";
    }
}

ActionCoverageReport BuildActionCoverageReport(
    const SkeletonDesc& skeleton,
    const ActionLibrary& library) {
    ActionCoverageReport report{};
    report.skeletonBoneCount = static_cast<int>(skeleton.bones.size());

    const std::array<PetAction, 7> expectedActions = {
        PetAction::Idle,
        PetAction::Follow,
        PetAction::ClickReact,
        PetAction::Drag,
        PetAction::HoverReact,
        PetAction::HoldReact,
        PetAction::ScrollReact,
    };

    report.expectedActionCount = static_cast<int>(expectedActions.size());
    report.actions.reserve(expectedActions.size());

    for (PetAction action : expectedActions) {
        ActionCoverageEntry entry{};
        entry.action = action;
        entry.actionName = PetActionName(action);

        const ActionClip* clip = library.FindClip(action);
        entry.clipPresent = (clip != nullptr);
        if (!clip) {
            report.missingActionCount += 1;
            report.missingActions.push_back(entry.actionName);
            report.actions.push_back(std::move(entry));
            continue;
        }

        report.coveredActionCount += 1;
        entry.trackCount = static_cast<int>(clip->tracks.size());
        report.totalTrackCount += entry.trackCount;

        for (const auto& track : clip->tracks) {
            if (ResolveTrackBoneIndex(skeleton, library, track.boneName) >= 0) {
                entry.mappedTrackCount += 1;
                continue;
            }
            entry.missingBoneTracks.push_back(track.boneName);
            AppendUnique(&report.missingBoneNames, track.boneName);
        }

        report.totalMappedTrackCount += entry.mappedTrackCount;
        if (entry.trackCount > 0) {
            entry.coverageRatio = static_cast<float>(entry.mappedTrackCount) /
                                  static_cast<float>(entry.trackCount);
        }

        report.actions.push_back(std::move(entry));
    }

    if (report.totalTrackCount > 0) {
        report.overallCoverageRatio = static_cast<float>(report.totalMappedTrackCount) /
                                      static_cast<float>(report.totalTrackCount);
    }

    return report;
}

} // namespace mousefx::pet
