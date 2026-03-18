#include "pch.h"
#include "MouseFx/Core/Pet/PetInterfaces.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace mousefx::pet {
namespace {

float Clamp01(double value) {
    if (value <= 0.0) {
        return 0.0f;
    }
    if (value >= 1.0) {
        return 1.0f;
    }
    return static_cast<float>(value);
}

float ClampRange(float value, float minValue, float maxValue) {
    return std::min(maxValue, std::max(minValue, value));
}

float Lerp(float a, float b, float alpha) {
    return a + (b - a) * ClampRange(alpha, 0.0f, 1.0f);
}

std::string NormalizeToken(std::string value) {
    std::transform(value.begin(),
                   value.end(),
                   value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

bool ContainsTokenCaseInsensitive(const std::string& text, const char* token) {
    if (!token || !token[0] || text.empty()) {
        return false;
    }
    return NormalizeToken(text).find(NormalizeToken(token)) != std::string::npos;
}

bool ContainsEitherDirection(const std::string& lhs, const std::string& rhs) {
    if (lhs.empty() || rhs.empty()) {
        return false;
    }
    return lhs.find(rhs) != std::string::npos || rhs.find(lhs) != std::string::npos;
}

int32_t FindBoneIndexByNormalizedToken(const SkeletonDesc* skeleton, const std::string& normalizedToken) {
    if (!skeleton || normalizedToken.empty()) {
        return -1;
    }
    for (size_t i = 0; i < skeleton->bones.size(); ++i) {
        const std::string normalizedBone = NormalizeToken(skeleton->bones[i].name);
        if (ContainsEitherDirection(normalizedBone, normalizedToken)) {
            return static_cast<int32_t>(i);
        }
    }
    return -1;
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

int32_t FindBoneIndexByTokens(const SkeletonDesc* skeleton, std::initializer_list<const char*> tokens) {
    if (!skeleton) {
        return -1;
    }
    for (const char* token : tokens) {
        for (size_t i = 0; i < skeleton->bones.size(); ++i) {
            if (ContainsTokenCaseInsensitive(skeleton->bones[i].name, token)) {
                return static_cast<int32_t>(i);
            }
        }
    }
    return -1;
}

int ActionKey(PetAction action) {
    return static_cast<int>(action);
}

Quat QuatFromEulerXYZ(float xRadians, float yRadians, float zRadians) {
    const float cx = std::cos(xRadians * 0.5f);
    const float sx = std::sin(xRadians * 0.5f);
    const float cy = std::cos(yRadians * 0.5f);
    const float sy = std::sin(yRadians * 0.5f);
    const float cz = std::cos(zRadians * 0.5f);
    const float sz = std::sin(zRadians * 0.5f);

    Quat q{};
    q.w = cx * cy * cz + sx * sy * sz;
    q.x = sx * cy * cz - cx * sy * sz;
    q.y = cx * sy * cz + sx * cy * sz;
    q.z = cx * cy * sz - sx * sy * cz;
    return q;
}

Quat NormalizeQuat(Quat q) {
    const float lenSq = q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w;
    if (lenSq <= 1e-8f) {
        return Quat{};
    }
    const float invLen = 1.0f / std::sqrt(lenSq);
    q.x *= invLen;
    q.y *= invLen;
    q.z *= invLen;
    q.w *= invLen;
    return q;
}

Quat NlerpQuat(const Quat& a, const Quat& b, float alpha) {
    Quat bAdjusted = b;
    const float dot = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    if (dot < 0.0f) {
        bAdjusted.x = -bAdjusted.x;
        bAdjusted.y = -bAdjusted.y;
        bAdjusted.z = -bAdjusted.z;
        bAdjusted.w = -bAdjusted.w;
    }
    Quat out{};
    out.x = Lerp(a.x, bAdjusted.x, alpha);
    out.y = Lerp(a.y, bAdjusted.y, alpha);
    out.z = Lerp(a.z, bAdjusted.z, alpha);
    out.w = Lerp(a.w, bAdjusted.w, alpha);
    return NormalizeQuat(out);
}

Vec3 LerpVec3(const Vec3& a, const Vec3& b, float alpha) {
    Vec3 out{};
    out.x = Lerp(a.x, b.x, alpha);
    out.y = Lerp(a.y, b.y, alpha);
    out.z = Lerp(a.z, b.z, alpha);
    return out;
}

Transform LerpTransform(const Transform& a, const Transform& b, float alpha) {
    Transform out{};
    out.position = LerpVec3(a.position, b.position, alpha);
    out.rotation = NlerpQuat(a.rotation, b.rotation, alpha);
    out.scale = LerpVec3(a.scale, b.scale, alpha);
    return out;
}

class DefaultActionSynthesizer final : public IActionSynthesizer {
public:
    void BindSkeleton(const SkeletonDesc* skeleton) override {
        skeleton_ = skeleton;
        boneIndexByName_.clear();
        if (skeleton_) {
            boneIndexByName_.reserve(skeleton_->bones.size());
            for (size_t i = 0; i < skeleton_->bones.size(); ++i) {
                const std::string normalized = NormalizeToken(skeleton_->bones[i].name);
                if (!normalized.empty()) {
                    boneIndexByName_[normalized] = static_cast<int32_t>(i);
                }
            }
        }
        rig_.hips = FindBoneIndexByTokens(skeleton, {"hips", "pelvis", "root", "center"});
        rig_.spine = FindBoneIndexByTokens(skeleton, {"spine", "spine_01", "spine1"});
        rig_.chest = FindBoneIndexByTokens(skeleton, {"chest", "upperchest", "spine_03", "spine2"});
        rig_.neck = FindBoneIndexByTokens(skeleton, {"neck"});
        rig_.head = FindBoneIndexByTokens(skeleton, {"head"});
        RebuildClipBindings();
    }

    void SetActionLibrary(std::shared_ptr<const ActionLibrary> actionLibrary) override {
        actionLibrary_ = std::move(actionLibrary);
        RebuildClipBindings();
    }

    void SetEffectProfile(std::shared_ptr<const ProceduralEffectProfile> effectProfile) override {
        if (effectProfile) {
            effectProfile_ = std::move(effectProfile);
            return;
        }
        effectProfile_ = CreateDefaultProceduralEffectProfile();
    }

    void RequestAction(PetAction action, double blendDurationSeconds) override {
        nextAction_ = action;
        blendDurationSeconds_ = std::max(0.0, blendDurationSeconds);
        blendProgressSeconds_ = 0.0;
    }

    void Update(const PetFrameInput& input, SkeletonPose* outPose) override {
        if (!outPose) {
            return;
        }

        const double dt = std::max(0.0, input.dtSeconds);
        timeSeconds_ += dt;

        float blendAlpha = 1.0f;
        if (nextAction_ != currentAction_) {
            if (blendDurationSeconds_ <= 0.0) {
                currentAction_ = nextAction_;
                blendProgressSeconds_ = blendDurationSeconds_;
            } else {
                blendProgressSeconds_ = std::min(blendDurationSeconds_, blendProgressSeconds_ + dt);
                blendAlpha = static_cast<float>(blendProgressSeconds_ / blendDurationSeconds_);
                if (blendProgressSeconds_ >= blendDurationSeconds_) {
                    currentAction_ = nextAction_;
                    blendAlpha = 1.0f;
                }
            }
        }
        const PetAction styleAction =
            (nextAction_ != currentAction_ && blendAlpha < 1.0f && blendAlpha >= 0.5f)
                ? nextAction_
                : currentAction_;
        if (styleAction != styleAction_) {
            styleAction_ = styleAction;
            styleElapsedSeconds_ = 0.0;
        } else {
            styleElapsedSeconds_ += dt;
        }

        outPose->bones.clear();
        outPose->actionIntensity = ResolveBlendedMetric(
            ResolveActionIntensity(currentAction_),
            ResolveActionIntensity(nextAction_),
            blendAlpha);
        outPose->locomotionForward = ResolveBlendedMetric(
            ResolveForward(currentAction_, input),
            ResolveForward(nextAction_, input),
            blendAlpha);
        outPose->locomotionTurn = ResolveTurn(input);

        const bool clipApplied = EmitClipPose(styleAction, outPose);
        if (!clipApplied) {
            EmitProceduralPose(styleAction, input, outPose);
            return;
        }

        const ProceduralActionSpec& spec = ResolveActionSpec(styleAction);
        const float clipBlendWeight = ClampRange(spec.clipBlendWeight, 0.0f, 1.0f);
        const float proceduralBlendWeight = 1.0f - clipBlendWeight;
        if (proceduralBlendWeight <= 0.001f) {
            return;
        }

        SkeletonPose proceduralPose{};
        EmitProceduralPose(styleAction, input, &proceduralPose);
        BlendPoseWithProcedural(proceduralBlendWeight, proceduralPose, outPose);
    }

private:
    struct RigSlots final {
        int32_t hips{-1};
        int32_t spine{-1};
        int32_t chest{-1};
        int32_t neck{-1};
        int32_t head{-1};
    };

    struct TrackBinding final {
        int32_t boneIndex{-1};
        const ActionBoneTrack* track{nullptr};
    };

    struct ClipBinding final {
        const ActionClip* clip{nullptr};
        std::vector<TrackBinding> tracks{};
    };

    void RebuildClipBindings() {
        clipBindings_.clear();
        if (!skeleton_ || !actionLibrary_) {
            return;
        }

        for (const auto& clip : actionLibrary_->clips) {
            ClipBinding binding{};
            binding.clip = &clip;
            for (const auto& track : clip.tracks) {
                const int32_t boneIndex = ResolveBoneIndexByName(track.boneName);
                if (boneIndex < 0) {
                    continue;
                }
                binding.tracks.push_back(TrackBinding{boneIndex, &track});
            }
            if (!binding.tracks.empty()) {
                clipBindings_[ActionKey(clip.action)] = std::move(binding);
            }
        }
    }

    int32_t ResolveBoneIndexByName(const std::string& boneName) const {
        if (boneName.empty() || !skeleton_) {
            return -1;
        }
        const std::string normalized = NormalizeToken(boneName);
        const auto byName = boneIndexByName_.find(normalized);
        if (byName != boneIndexByName_.end()) {
            return byName->second;
        }

        const int32_t byContains = FindBoneIndexByNormalizedToken(skeleton_, normalized);
        if (byContains >= 0) {
            return byContains;
        }

        const std::vector<std::string> aliases = ResolveBoneAliases(normalized);
        for (const auto& alias : aliases) {
            const int32_t byAlias = FindBoneIndexByNormalizedToken(skeleton_, alias);
            if (byAlias >= 0) {
                return byAlias;
            }
        }

        if (actionLibrary_) {
            const ActionBoneRemapRule* remap = actionLibrary_->FindBoneRemap(boneName);
            if (remap) {
                for (const auto& candidateBone : remap->candidateBones) {
                    const std::string normalizedCandidate = NormalizeToken(candidateBone);
                    const auto byCandidateName = boneIndexByName_.find(normalizedCandidate);
                    if (byCandidateName != boneIndexByName_.end()) {
                        return byCandidateName->second;
                    }
                    const int32_t byCandidateContains =
                        FindBoneIndexByNormalizedToken(skeleton_, normalizedCandidate);
                    if (byCandidateContains >= 0) {
                        return byCandidateContains;
                    }
                }
            }
        }
        return -1;
    }

    bool EmitClipPose(PetAction styleAction, SkeletonPose* outPose) const {
        if (!outPose || !skeleton_ || skeleton_->bones.empty()) {
            return false;
        }
        const auto clipIt = clipBindings_.find(ActionKey(styleAction));
        if (clipIt == clipBindings_.end() || !clipIt->second.clip || clipIt->second.tracks.empty()) {
            return false;
        }

        const ActionClip& clip = *clipIt->second.clip;
        const float clipTime = ResolveClipTime(clip);
        std::vector<uint8_t> assigned(skeleton_->bones.size(), 0);
        outPose->bones.reserve(clipIt->second.tracks.size());

        for (const auto& trackBinding : clipIt->second.tracks) {
            if (!trackBinding.track || trackBinding.boneIndex < 0) {
                continue;
            }
            const Transform local = SampleTrackPose(*trackBinding.track, clipTime);
            PushBonePose(trackBinding.boneIndex, local, &assigned, outPose);
        }
        return !outPose->bones.empty();
    }

    float ResolveClipTime(const ActionClip& clip) const {
        const float elapsed = std::max(0.0f, static_cast<float>(styleElapsedSeconds_));
        if (clip.durationSeconds <= 0.0f) {
            return elapsed;
        }
        if (clip.loop) {
            const float wrapped = std::fmod(elapsed, clip.durationSeconds);
            return (wrapped >= 0.0f) ? wrapped : wrapped + clip.durationSeconds;
        }
        return std::min(elapsed, clip.durationSeconds);
    }

    Transform SampleTrackPose(const ActionBoneTrack& track, float clipTime) const {
        if (track.keyframes.empty()) {
            return Transform{};
        }
        if (track.keyframes.size() == 1) {
            return track.keyframes.front().local;
        }
        if (clipTime <= track.keyframes.front().timeSeconds) {
            return track.keyframes.front().local;
        }
        for (size_t i = 1; i < track.keyframes.size(); ++i) {
            const ActionBoneKeyframe& left = track.keyframes[i - 1];
            const ActionBoneKeyframe& right = track.keyframes[i];
            if (clipTime > right.timeSeconds) {
                continue;
            }
            if (left.interpolation == ClipInterpolation::Step ||
                right.timeSeconds <= left.timeSeconds + 1e-5f) {
                return left.local;
            }
            const float alpha = (clipTime - left.timeSeconds) / (right.timeSeconds - left.timeSeconds);
            return LerpTransform(left.local, right.local, alpha);
        }
        return track.keyframes.back().local;
    }

    void EmitProceduralPose(PetAction styleAction, const PetFrameInput& input, SkeletonPose* outPose) const {
        if (!outPose || !skeleton_ || skeleton_->bones.empty()) {
            return;
        }

        std::vector<uint8_t> assigned(skeleton_->bones.size(), 0);
        const ProceduralActionSpec& spec = ResolveActionSpec(styleAction);
        const float actionIntensity = ClampRange(spec.actionIntensity, 0.0f, 1.0f);
        const float turn = ClampRange(outPose->locomotionTurn, -1.0f, 1.0f);
        const float breathe =
            std::sin(static_cast<float>(timeSeconds_) * std::max(0.1f, spec.breatheHz)) * spec.breatheAmplitude;
        const float pulse = (spec.pulseGain > 0.001f)
            ? std::exp(-static_cast<float>(styleElapsedSeconds_) * std::max(0.1f, spec.pulseDecayHz)) *
                  spec.pulseGain
            : 0.0f;

        Transform hips{};
        hips.position.y = breathe * 0.08f + spec.hipsYOffset;
        hips.rotation = QuatFromEulerXYZ(
            spec.hipsPitch + 0.05f * pulse,
            turn * (0.08f + actionIntensity * 0.06f) + spec.hipsYaw,
            0.0f);
        const float pulseScale = pulse * 0.08f;
        hips.scale = {
            std::max(0.70f, spec.hipsScaleX + pulseScale),
            std::max(0.70f, spec.hipsScaleY - pulse * 0.10f),
            std::max(0.70f, spec.hipsScaleZ + pulseScale)};
        PushBonePose(rig_.hips, hips, &assigned, outPose);

        Transform spine{};
        spine.rotation = QuatFromEulerXYZ(
            spec.spinePitch + breathe * 0.30f,
            turn * (0.12f + actionIntensity * 0.08f) + spec.spineYaw,
            0.0f);
        PushBonePose(rig_.spine, spine, &assigned, outPose);

        Transform chest{};
        chest.rotation = QuatFromEulerXYZ(
            spec.chestPitch + breathe * 0.45f,
            turn * (0.20f + actionIntensity * 0.12f) + spec.chestYaw,
            0.0f);
        PushBonePose(rig_.chest, chest, &assigned, outPose);

        Transform neck{};
        neck.rotation = QuatFromEulerXYZ(
            spec.neckPitch + breathe * 0.40f,
            turn * 0.28f + spec.neckYaw,
            0.0f);
        PushBonePose(rig_.neck, neck, &assigned, outPose);

        const float cursorPitch = ClampRange(-input.cursorPosition.y * 0.0012f, -0.30f, 0.28f);
        Transform head{};
        head.rotation = QuatFromEulerXYZ(
            cursorPitch * spec.headCursorPitchGain + spec.headPitchBias + breathe * 0.20f,
            turn * spec.headYawGain + spec.headYawBias,
            0.0f);
        PushBonePose(rig_.head, head, &assigned, outPose);
    }

    void BlendPoseWithProcedural(
        float proceduralWeight,
        const SkeletonPose& proceduralPose,
        SkeletonPose* inOutPose) const {
        if (!inOutPose) {
            return;
        }
        const float weight = ClampRange(proceduralWeight, 0.0f, 1.0f);
        if (weight <= 0.001f || proceduralPose.bones.empty()) {
            return;
        }

        for (const BonePose& source : proceduralPose.bones) {
            if (source.boneIndex < 0) {
                continue;
            }

            BonePose* target = nullptr;
            for (auto& existing : inOutPose->bones) {
                if (existing.boneIndex == source.boneIndex) {
                    target = &existing;
                    break;
                }
            }
            if (target) {
                target->local = LerpTransform(target->local, source.local, weight);
                continue;
            }

            BonePose injected = source;
            injected.local = LerpTransform(Transform{}, source.local, weight);
            inOutPose->bones.push_back(std::move(injected));
        }
    }

    void PushBonePose(int32_t boneIndex,
                      const Transform& local,
                      std::vector<uint8_t>* inOutAssigned,
                      SkeletonPose* outPose) const {
        if (!outPose || !skeleton_ || boneIndex < 0) {
            return;
        }
        const size_t index = static_cast<size_t>(boneIndex);
        if (index >= skeleton_->bones.size()) {
            return;
        }
        if (inOutAssigned && index < inOutAssigned->size()) {
            if ((*inOutAssigned)[index] != 0) {
                return;
            }
            (*inOutAssigned)[index] = 1;
        }
        BonePose pose{};
        pose.boneIndex = boneIndex;
        pose.name = skeleton_->bones[index].name;
        pose.local = local;
        outPose->bones.push_back(std::move(pose));
    }

    static float ResolveBlendedMetric(float currentValue, float targetValue, float blendAlpha) {
        if (blendAlpha <= 0.0f) {
            return currentValue;
        }
        if (blendAlpha >= 1.0f) {
            return targetValue;
        }
        return Lerp(currentValue, targetValue, blendAlpha);
    }

    float ResolveActionIntensity(PetAction action) const {
        return ResolveActionSpec(action).actionIntensity;
    }

    static float ResolveForward(PetAction action, const PetFrameInput& input) {
        switch (action) {
        case PetAction::Follow:
            return input.cursorVisible ? 1.0f : 0.0f;
        case PetAction::Drag:
            return input.dragging ? 0.35f : 0.0f;
        case PetAction::HoverReact:
            return input.cursorVisible ? 0.45f : 0.0f;
        case PetAction::HoldReact:
            return input.primaryPressed ? 0.25f : 0.0f;
        case PetAction::ScrollReact:
            return input.cursorVisible ? 0.65f : 0.0f;
        default:
            return 0.0f;
        }
    }

    static float ResolveTurn(const PetFrameInput& input) {
        constexpr float kTurnScale = 0.002f;
        return Clamp01(0.5 + (input.cursorPosition.x * kTurnScale)) * 2.0f - 1.0f;
    }

    const ProceduralActionSpec& ResolveActionSpec(PetAction action) const {
        if (effectProfile_) {
            if (const auto* spec = effectProfile_->Find(action)) {
                return *spec;
            }
        }
        static const ProceduralActionSpec kFallback{};
        return kFallback;
    }

    const SkeletonDesc* skeleton_{nullptr};
    std::unordered_map<std::string, int32_t> boneIndexByName_{};
    RigSlots rig_{};
    std::shared_ptr<const ActionLibrary> actionLibrary_{};
    std::shared_ptr<const ProceduralEffectProfile> effectProfile_{CreateDefaultProceduralEffectProfile()};
    std::unordered_map<int, ClipBinding> clipBindings_{};
    PetAction currentAction_{PetAction::Idle};
    PetAction nextAction_{PetAction::Idle};
    PetAction styleAction_{PetAction::Idle};
    double styleElapsedSeconds_{0.0};
    double blendDurationSeconds_{0.0};
    double blendProgressSeconds_{0.0};
    double timeSeconds_{0.0};
};

} // namespace

std::unique_ptr<IActionSynthesizer> CreateDefaultActionSynthesizer() {
    return std::make_unique<DefaultActionSynthesizer>();
}

} // namespace mousefx::pet
