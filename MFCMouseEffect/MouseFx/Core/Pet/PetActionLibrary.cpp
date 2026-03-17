#include "pch.h"
#include "MouseFx/Core/Pet/PetActionLibrary.h"

#include "MouseFx/Core/Json/JsonFacade.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace mousefx::pet {
namespace {

std::string NormalizeToken(std::string value) {
    std::transform(value.begin(),
                   value.end(),
                   value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

bool ReadJsonFile(const std::string& path, std::string* outText) {
    if (!outText) {
        return false;
    }
    std::ifstream in(std::filesystem::path(path), std::ios::binary);
    if (!in) {
        return false;
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    *outText = ss.str();
    return !outText->empty();
}

bool ParseActionName(const std::string& raw, PetAction* outAction) {
    if (!outAction) {
        return false;
    }
    const std::string normalized = NormalizeToken(raw);
    if (normalized == "idle") {
        *outAction = PetAction::Idle;
        return true;
    }
    if (normalized == "follow") {
        *outAction = PetAction::Follow;
        return true;
    }
    if (normalized == "clickreact" || normalized == "click_react" || normalized == "click") {
        *outAction = PetAction::ClickReact;
        return true;
    }
    if (normalized == "drag") {
        *outAction = PetAction::Drag;
        return true;
    }
    return false;
}

ClipInterpolation ParseInterpolation(const nlohmann::json& keyframeNode) {
    const auto it = keyframeNode.find("interp");
    if (it == keyframeNode.end() || !it->is_string()) {
        return ClipInterpolation::Linear;
    }
    const std::string value = NormalizeToken(it->get<std::string>());
    if (value == "step") {
        return ClipInterpolation::Step;
    }
    return ClipInterpolation::Linear;
}

bool ParseFloatArray(const nlohmann::json& node,
                     const char* key,
                     size_t expectedCount,
                     std::vector<float>* outValues) {
    if (!outValues) {
        return false;
    }
    outValues->clear();

    const auto it = node.find(key);
    if (it == node.end()) {
        return false;
    }
    if (!it->is_array() || it->size() != expectedCount) {
        return false;
    }

    outValues->reserve(expectedCount);
    for (const auto& value : *it) {
        if (!value.is_number()) {
            return false;
        }
        const float parsed = value.get<float>();
        if (!std::isfinite(parsed)) {
            return false;
        }
        outValues->push_back(parsed);
    }
    return outValues->size() == expectedCount;
}

bool ParseKeyframe(const nlohmann::json& keyframeNode, ActionBoneKeyframe* outKeyframe) {
    if (!outKeyframe || !keyframeNode.is_object()) {
        return false;
    }
    const auto tIt = keyframeNode.find("t");
    if (tIt == keyframeNode.end() || !tIt->is_number()) {
        return false;
    }

    ActionBoneKeyframe keyframe{};
    keyframe.timeSeconds = std::max(0.0f, tIt->get<float>());
    keyframe.interpolation = ParseInterpolation(keyframeNode);

    std::vector<float> values;
    if (ParseFloatArray(keyframeNode, "pos", 3, &values)) {
        keyframe.local.position.x = values[0];
        keyframe.local.position.y = values[1];
        keyframe.local.position.z = values[2];
    }
    if (ParseFloatArray(keyframeNode, "rot", 4, &values)) {
        keyframe.local.rotation.x = values[0];
        keyframe.local.rotation.y = values[1];
        keyframe.local.rotation.z = values[2];
        keyframe.local.rotation.w = values[3];
    }
    if (ParseFloatArray(keyframeNode, "scale", 3, &values)) {
        keyframe.local.scale.x = values[0];
        keyframe.local.scale.y = values[1];
        keyframe.local.scale.z = values[2];
    }

    *outKeyframe = std::move(keyframe);
    return true;
}

bool ParseTrack(const nlohmann::json& trackNode, ActionBoneTrack* outTrack) {
    if (!outTrack || !trackNode.is_object()) {
        return false;
    }
    const auto boneIt = trackNode.find("bone");
    const auto keyframesIt = trackNode.find("keyframes");
    if (boneIt == trackNode.end() || !boneIt->is_string() ||
        keyframesIt == trackNode.end() || !keyframesIt->is_array()) {
        return false;
    }

    ActionBoneTrack track{};
    track.boneName = boneIt->get<std::string>();
    if (track.boneName.empty()) {
        return false;
    }

    track.keyframes.reserve(keyframesIt->size());
    for (const auto& keyframeNode : *keyframesIt) {
        ActionBoneKeyframe keyframe{};
        if (!ParseKeyframe(keyframeNode, &keyframe)) {
            continue;
        }
        track.keyframes.push_back(std::move(keyframe));
    }
    if (track.keyframes.empty()) {
        return false;
    }

    std::sort(track.keyframes.begin(),
              track.keyframes.end(),
              [](const ActionBoneKeyframe& a, const ActionBoneKeyframe& b) {
                  return a.timeSeconds < b.timeSeconds;
              });
    *outTrack = std::move(track);
    return true;
}

void AppendUniqueNonEmpty(std::vector<std::string>* values, const std::string& item) {
    if (!values) {
        return;
    }
    const std::string trimmed = item;
    if (trimmed.empty()) {
        return;
    }
    for (const auto& existing : *values) {
        if (NormalizeToken(existing) == NormalizeToken(trimmed)) {
            return;
        }
    }
    values->push_back(trimmed);
}

void ParseBoneRemapRules(const nlohmann::json& root, ActionLibrary* library) {
    if (!library) {
        return;
    }
    library->boneRemaps.clear();

    const auto remapIt = root.find("bone_remap");
    if (remapIt == root.end() || !remapIt->is_object()) {
        return;
    }

    for (auto it = remapIt->begin(); it != remapIt->end(); ++it) {
        if (it.key().empty()) {
            continue;
        }
        ActionBoneRemapRule rule{};
        rule.sourceBone = it.key();

        if (it.value().is_string()) {
            AppendUniqueNonEmpty(&rule.candidateBones, it.value().get<std::string>());
        } else if (it.value().is_array()) {
            for (const auto& candidate : it.value()) {
                if (!candidate.is_string()) {
                    continue;
                }
                AppendUniqueNonEmpty(&rule.candidateBones, candidate.get<std::string>());
            }
        }

        if (rule.candidateBones.empty()) {
            continue;
        }
        library->boneRemaps.push_back(std::move(rule));
    }
}

bool ParseClip(const nlohmann::json& clipNode, ActionClip* outClip) {
    if (!outClip || !clipNode.is_object()) {
        return false;
    }
    const auto actionIt = clipNode.find("action");
    const auto tracksIt = clipNode.find("tracks");
    if (actionIt == clipNode.end() || !actionIt->is_string() ||
        tracksIt == clipNode.end() || !tracksIt->is_array()) {
        return false;
    }

    ActionClip clip{};
    if (!ParseActionName(actionIt->get<std::string>(), &clip.action)) {
        return false;
    }

    const auto durationIt = clipNode.find("duration");
    if (durationIt != clipNode.end() && durationIt->is_number()) {
        clip.durationSeconds = std::max(0.0f, durationIt->get<float>());
    }

    const auto loopIt = clipNode.find("loop");
    if (loopIt != clipNode.end() && loopIt->is_boolean()) {
        clip.loop = loopIt->get<bool>();
    }

    clip.tracks.reserve(tracksIt->size());
    float maxTrackTime = 0.0f;
    for (const auto& trackNode : *tracksIt) {
        ActionBoneTrack track{};
        if (!ParseTrack(trackNode, &track)) {
            continue;
        }
        maxTrackTime = std::max(maxTrackTime, track.keyframes.back().timeSeconds);
        clip.tracks.push_back(std::move(track));
    }
    if (clip.tracks.empty()) {
        return false;
    }

    if (clip.durationSeconds <= 0.0f) {
        clip.durationSeconds = std::max(0.001f, maxTrackTime);
    }
    *outClip = std::move(clip);
    return true;
}

} // namespace

const ActionClip* ActionLibrary::FindClip(PetAction action) const {
    for (const auto& clip : clips) {
        if (clip.action == action) {
            return &clip;
        }
    }
    return nullptr;
}

const ActionBoneRemapRule* ActionLibrary::FindBoneRemap(const std::string& sourceBone) const {
    if (sourceBone.empty()) {
        return nullptr;
    }
    const std::string normalizedSource = NormalizeToken(sourceBone);
    for (const auto& rule : boneRemaps) {
        if (NormalizeToken(rule.sourceBone) == normalizedSource) {
            return &rule;
        }
    }
    return nullptr;
}

bool LoadActionLibraryFromJsonFile(const std::string& jsonPath,
                                   ActionLibrary* outLibrary,
                                   std::string* outError) {
    if (outError) {
        outError->clear();
    }
    if (!outLibrary) {
        if (outError) {
            *outError = "action library output pointer is null";
        }
        return false;
    }

    std::string text;
    if (!ReadJsonFile(jsonPath, &text)) {
        if (outError) {
            *outError = "failed to read action library json";
        }
        return false;
    }

    nlohmann::json root;
    try {
        root = nlohmann::json::parse(text);
    } catch (const nlohmann::json::exception& ex) {
        if (outError) {
            *outError = std::string("action library json parse failed: ") + ex.what();
        }
        return false;
    }

    const auto clipsIt = root.find("clips");
    if (clipsIt == root.end() || !clipsIt->is_array() || clipsIt->empty()) {
        if (outError) {
            *outError = "action library json missing clips";
        }
        return false;
    }

    ActionLibrary parsed{};
    parsed.clips.reserve(clipsIt->size());
    for (const auto& clipNode : *clipsIt) {
        ActionClip clip{};
        if (!ParseClip(clipNode, &clip)) {
            continue;
        }
        parsed.clips.push_back(std::move(clip));
    }

    if (parsed.clips.empty()) {
        if (outError) {
            *outError = "no valid clips in action library";
        }
        return false;
    }

    ParseBoneRemapRules(root, &parsed);

    *outLibrary = std::move(parsed);
    return true;
}

} // namespace mousefx::pet
