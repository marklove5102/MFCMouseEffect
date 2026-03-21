#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionActionLibrary.h"

#include "MouseFx/Core/Json/JsonFacade.h"
#include "MouseFx/Utils/StringUtils.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>

namespace mousefx::windows {
namespace {

using json = nlohmann::json;

std::string NormalizeKey(std::string value) {
    value = ToLowerAscii(TrimAscii(value));
    value.erase(
        std::remove_if(
            value.begin(),
            value.end(),
            [](char ch) { return ch == '_' || ch == '-' || ch == ' '; }),
        value.end());
    return value;
}

bool ParseFloatArray(
    const json& source,
    const char* key,
    size_t expectedSize,
    float* outValues) {
    if (!outValues || !source.contains(key) || !source[key].is_array() || source[key].size() != expectedSize) {
        return false;
    }
    for (size_t i = 0; i < expectedSize; ++i) {
        if (!source[key][i].is_number()) {
            return false;
        }
        outValues[i] = source[key][i].get<float>();
    }
    return true;
}

Win32MouseCompanionActionKeyframe SampleTrackKeyframe(
    const Win32MouseCompanionActionTrack& track,
    float tSec) {
    if (track.keyframes.empty()) {
        return {};
    }
    if (track.keyframes.size() == 1 || tSec <= track.keyframes.front().t) {
        return track.keyframes.front();
    }
    if (tSec >= track.keyframes.back().t) {
        return track.keyframes.back();
    }

    for (size_t i = 1; i < track.keyframes.size(); ++i) {
        const auto& lhs = track.keyframes[i - 1];
        const auto& rhs = track.keyframes[i];
        if (tSec > rhs.t) {
            continue;
        }
        const float span = std::max(0.0001f, rhs.t - lhs.t);
        const float alpha = std::clamp((tSec - lhs.t) / span, 0.0f, 1.0f);
        Win32MouseCompanionActionKeyframe sample{};
        sample.t = tSec;
        sample.hasScale = lhs.hasScale || rhs.hasScale;
        sample.hasRotation = lhs.hasRotation || rhs.hasRotation;
        for (size_t c = 0; c < 3; ++c) {
            const float lhsScale = lhs.hasScale ? lhs.scale[c] : 1.0f;
            const float rhsScale = rhs.hasScale ? rhs.scale[c] : 1.0f;
            sample.scale[c] = lhsScale + (rhsScale - lhsScale) * alpha;
        }
        if (sample.hasRotation) {
            std::array<float, 4> q{};
            for (size_t c = 0; c < 4; ++c) {
                const float lhsRot = lhs.hasRotation ? lhs.rotation[c] : (c == 3 ? 1.0f : 0.0f);
                const float rhsRot = rhs.hasRotation ? rhs.rotation[c] : (c == 3 ? 1.0f : 0.0f);
                q[c] = lhsRot + (rhsRot - lhsRot) * alpha;
            }
            const float length = std::sqrt(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
            if (length > 0.0001f) {
                for (size_t c = 0; c < 4; ++c) {
                    sample.rotation[c] = q[c] / length;
                }
            }
        }
        return sample;
    }
    return track.keyframes.back();
}

void QuaternionToEulerXY(
    const float q[4],
    float* outPitch,
    float* outYaw) {
    if (outPitch) {
        *outPitch = 0.0f;
    }
    if (outYaw) {
        *outYaw = 0.0f;
    }
    if (!q) {
        return;
    }
    const double x = q[0];
    const double y = q[1];
    const double z = q[2];
    const double w = q[3];

    const double sinPitch = 2.0 * (w * x + y * z);
    const double cosPitch = 1.0 - 2.0 * (x * x + y * y);
    const double pitch = std::atan2(sinPitch, cosPitch);

    double sinYaw = 2.0 * (w * y - z * x);
    sinYaw = std::clamp(sinYaw, -1.0, 1.0);
    const double yaw = std::asin(sinYaw);

    if (outPitch) {
        *outPitch = static_cast<float>(pitch);
    }
    if (outYaw) {
        *outYaw = static_cast<float>(yaw);
    }
}

bool IsBodyBone(const std::string& bone) {
    const std::string key = NormalizeKey(bone);
    return key == "chest" || key == "spine" || key == "spine2" || key == "upperchest";
}

bool IsHeadBone(const std::string& bone) {
    return NormalizeKey(bone) == "head";
}

} // namespace

bool LoadWin32MouseCompanionActionLibraryFromPath(
    const std::string& path,
    Win32MouseCompanionActionLibrary* outLibrary) {
    if (!outLibrary) {
        return false;
    }
    *outLibrary = {};

    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }

    std::string content(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());
    json root;
    try {
        root = json::parse(content);
    } catch (...) {
        return false;
    }
    if (!root.contains("clips") || !root["clips"].is_array()) {
        return false;
    }

    for (const auto& clipNode : root["clips"]) {
        if (!clipNode.is_object()) {
            continue;
        }
        const std::string action = clipNode.value("action", "");
        const std::string key = NormalizeKey(action);
        if (key.empty()) {
            continue;
        }

        Win32MouseCompanionActionClip clip{};
        clip.action = action;
        clip.durationSec = std::max(0.0f, clipNode.value("duration", 0.0f));
        clip.loop = clipNode.value("loop", false);

        if (!clipNode.contains("tracks") || !clipNode["tracks"].is_array()) {
            continue;
        }
        for (const auto& trackNode : clipNode["tracks"]) {
            if (!trackNode.is_object()) {
                continue;
            }
            Win32MouseCompanionActionTrack track{};
            track.bone = trackNode.value("bone", "");
            if (track.bone.empty() ||
                !trackNode.contains("keyframes") ||
                !trackNode["keyframes"].is_array()) {
                continue;
            }

            for (const auto& keyframeNode : trackNode["keyframes"]) {
                if (!keyframeNode.is_object() || !keyframeNode.contains("t")) {
                    continue;
                }
                Win32MouseCompanionActionKeyframe keyframe{};
                keyframe.t = keyframeNode.value("t", 0.0f);
                keyframe.hasRotation = ParseFloatArray(keyframeNode, "rot", 4, keyframe.rotation);
                keyframe.hasScale = ParseFloatArray(keyframeNode, "scale", 3, keyframe.scale);
                if (keyframe.hasRotation || keyframe.hasScale) {
                    track.keyframes.push_back(keyframe);
                    clip.durationSec = std::max(clip.durationSec, keyframe.t);
                }
            }

            if (!track.keyframes.empty()) {
                std::sort(
                    track.keyframes.begin(),
                    track.keyframes.end(),
                    [](const auto& lhs, const auto& rhs) { return lhs.t < rhs.t; });
                clip.tracks.push_back(std::move(track));
            }
        }

        if (!clip.tracks.empty()) {
            outLibrary->clipsByAction[key] = std::move(clip);
        }
    }

    return !outLibrary->clipsByAction.empty();
}

bool SampleWin32MouseCompanionActionLibrary(
    const Win32MouseCompanionActionLibrary& library,
    const std::string& actionName,
    uint64_t actionElapsedMs,
    Win32MouseCompanionActionSample* outSample) {
    if (!outSample) {
        return false;
    }
    *outSample = {};

    const auto it = library.clipsByAction.find(NormalizeKey(actionName));
    if (it == library.clipsByAction.end()) {
        return false;
    }

    const auto& clip = it->second;
    if (clip.durationSec <= 0.0001f) {
        return false;
    }

    float sampleTimeSec = static_cast<float>(actionElapsedMs) / 1000.0f;
    if (clip.loop) {
        sampleTimeSec = std::fmod(sampleTimeSec, clip.durationSec);
        if (sampleTimeSec < 0.0f) {
            sampleTimeSec += clip.durationSec;
        }
    } else {
        sampleTimeSec = std::clamp(sampleTimeSec, 0.0f, clip.durationSec);
    }

    for (const auto& track : clip.tracks) {
        const auto sample = SampleTrackKeyframe(track, sampleTimeSec);
        if (IsBodyBone(track.bone)) {
            if (sample.hasScale) {
                outSample->bodyScaleX = sample.scale[0];
                outSample->bodyScaleY = sample.scale[1];
                outSample->valid = true;
            }
            if (sample.hasRotation) {
                QuaternionToEulerXY(sample.rotation, nullptr, &outSample->bodyLean);
                outSample->valid = true;
            }
        } else if (IsHeadBone(track.bone) && sample.hasRotation) {
            QuaternionToEulerXY(sample.rotation, &outSample->headPitch, &outSample->headYaw);
            outSample->valid = true;
        }
    }

    return outSample->valid;
}

} // namespace mousefx::windows
