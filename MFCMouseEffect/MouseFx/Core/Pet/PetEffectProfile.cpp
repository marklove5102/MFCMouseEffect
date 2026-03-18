#include "pch.h"
#include "MouseFx/Core/Pet/PetEffectProfile.h"

#include "MouseFx/Core/Json/JsonFacade.h"
#include "MouseFx/Utils/StringUtils.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace mousefx::pet {
namespace {

using json = nlohmann::json;

std::string NormalizeToken(std::string value) {
    std::transform(
        value.begin(),
        value.end(),
        value.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
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
    if (normalized == "hoverreact" || normalized == "hover_react" || normalized == "hover") {
        *outAction = PetAction::HoverReact;
        return true;
    }
    if (normalized == "holdreact" || normalized == "hold_react" || normalized == "hold") {
        *outAction = PetAction::HoldReact;
        return true;
    }
    if (normalized == "scrollreact" || normalized == "scroll_react" || normalized == "scroll") {
        *outAction = PetAction::ScrollReact;
        return true;
    }
    return false;
}

bool ReadJsonFile(const std::string& path, std::string* outText) {
    if (!outText) {
        return false;
    }
    std::ifstream in(std::filesystem::path(path), std::ios::binary);
    if (!in.is_open()) {
        return false;
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    *outText = ss.str();
    return !outText->empty();
}

void ParseFloatField(const json& node, const char* key, float* outValue) {
    if (!outValue || !key || !node.is_object()) {
        return;
    }
    const auto it = node.find(key);
    if (it == node.end() || !it->is_number()) {
        return;
    }
    *outValue = it->get<float>();
}

void ParseActionSpec(const json& node, ProceduralActionSpec* outSpec) {
    if (!outSpec || !node.is_object()) {
        return;
    }
    ParseFloatField(node, "action_intensity", &outSpec->actionIntensity);
    ParseFloatField(node, "clip_blend_weight", &outSpec->clipBlendWeight);
    ParseFloatField(node, "breathe_hz", &outSpec->breatheHz);
    ParseFloatField(node, "breathe_amplitude", &outSpec->breatheAmplitude);
    ParseFloatField(node, "pulse_decay_hz", &outSpec->pulseDecayHz);
    ParseFloatField(node, "pulse_gain", &outSpec->pulseGain);

    ParseFloatField(node, "hips_y_offset", &outSpec->hipsYOffset);
    ParseFloatField(node, "hips_pitch", &outSpec->hipsPitch);
    ParseFloatField(node, "hips_yaw", &outSpec->hipsYaw);
    ParseFloatField(node, "hips_scale_x", &outSpec->hipsScaleX);
    ParseFloatField(node, "hips_scale_y", &outSpec->hipsScaleY);
    ParseFloatField(node, "hips_scale_z", &outSpec->hipsScaleZ);

    ParseFloatField(node, "spine_pitch", &outSpec->spinePitch);
    ParseFloatField(node, "spine_yaw", &outSpec->spineYaw);
    ParseFloatField(node, "chest_pitch", &outSpec->chestPitch);
    ParseFloatField(node, "chest_yaw", &outSpec->chestYaw);
    ParseFloatField(node, "neck_pitch", &outSpec->neckPitch);
    ParseFloatField(node, "neck_yaw", &outSpec->neckYaw);

    ParseFloatField(node, "head_cursor_pitch_gain", &outSpec->headCursorPitchGain);
    ParseFloatField(node, "head_pitch_bias", &outSpec->headPitchBias);
    ParseFloatField(node, "head_yaw_gain", &outSpec->headYawGain);
    ParseFloatField(node, "head_yaw_bias", &outSpec->headYawBias);
}

void BuildDefaultProfile(ProceduralEffectProfile* outProfile) {
    if (!outProfile) {
        return;
    }
    outProfile->version = 1;
    outProfile->actions.clear();

    auto put = [&](PetAction action, const ProceduralActionSpec& spec) {
        outProfile->actions[static_cast<int>(action)] = spec;
    };

    ProceduralActionSpec idle{};
    idle.actionIntensity = 0.22f;
    idle.clipBlendWeight = 0.55f;
    idle.breatheHz = 2.2f;
    idle.breatheAmplitude = 0.036f;
    idle.spinePitch = -0.01f;
    idle.chestPitch = -0.01f;
    idle.headCursorPitchGain = 0.28f;
    idle.headYawGain = 0.34f;
    put(PetAction::Idle, idle);

    ProceduralActionSpec follow = idle;
    follow.actionIntensity = 0.62f;
    follow.clipBlendWeight = 0.45f;
    follow.spinePitch = -0.04f;
    follow.spineYaw = 0.14f;
    follow.chestPitch = -0.03f;
    follow.chestYaw = 0.22f;
    follow.headCursorPitchGain = 0.52f;
    follow.headYawGain = 0.58f;
    put(PetAction::Follow, follow);

    ProceduralActionSpec click = follow;
    click.actionIntensity = 0.92f;
    click.clipBlendWeight = 0.30f;
    click.pulseGain = 1.0f;
    click.pulseDecayHz = 9.0f;
    click.hipsPitch = 0.08f;
    click.hipsScaleX = 1.07f;
    click.hipsScaleY = 0.88f;
    click.hipsScaleZ = 1.07f;
    click.neckPitch = 0.08f;
    click.headPitchBias = -0.06f;
    put(PetAction::ClickReact, click);

    ProceduralActionSpec drag = follow;
    drag.actionIntensity = 0.82f;
    drag.clipBlendWeight = 0.28f;
    drag.hipsPitch = -0.09f;
    drag.hipsYaw = 0.10f;
    drag.spinePitch = -0.16f;
    drag.spineYaw = 0.18f;
    drag.chestPitch = -0.13f;
    drag.chestYaw = 0.26f;
    drag.headPitchBias = -0.14f;
    drag.headYawGain = 0.48f;
    put(PetAction::Drag, drag);

    ProceduralActionSpec hover = follow;
    hover.actionIntensity = 0.70f;
    hover.clipBlendWeight = 0.24f;
    hover.breatheHz = 2.8f;
    hover.hipsYOffset = 0.012f;
    hover.hipsYaw = 0.16f;
    hover.spineYaw = 0.28f;
    hover.chestYaw = 0.34f;
    hover.neckPitch = 0.08f;
    hover.neckYaw = 0.22f;
    hover.headPitchBias = 0.08f;
    hover.headYawBias = 0.20f;
    hover.headYawGain = 0.66f;
    put(PetAction::HoverReact, hover);

    ProceduralActionSpec hold = follow;
    hold.actionIntensity = 0.86f;
    hold.clipBlendWeight = 0.22f;
    hold.hipsYOffset = -0.018f;
    hold.hipsPitch = -0.10f;
    hold.hipsScaleX = 1.10f;
    hold.hipsScaleY = 0.80f;
    hold.hipsScaleZ = 1.10f;
    hold.spinePitch = -0.18f;
    hold.chestPitch = -0.20f;
    hold.headPitchBias = -0.18f;
    hold.neckPitch = -0.05f;
    hold.headYawGain = 0.42f;
    put(PetAction::HoldReact, hold);

    ProceduralActionSpec scroll = follow;
    scroll.actionIntensity = 0.78f;
    scroll.clipBlendWeight = 0.18f;
    scroll.breatheHz = 3.3f;
    scroll.hipsYaw = 0.26f;
    scroll.spineYaw = 0.38f;
    scroll.chestYaw = 0.46f;
    scroll.neckYaw = 0.36f;
    scroll.headYawBias = 0.28f;
    scroll.headYawGain = 0.84f;
    put(PetAction::ScrollReact, scroll);
}

} // namespace

const ProceduralActionSpec* ProceduralEffectProfile::Find(PetAction action) const {
    const auto it = actions.find(static_cast<int>(action));
    if (it == actions.end()) {
        return nullptr;
    }
    return &it->second;
}

bool LoadProceduralEffectProfileFromJsonFile(
    const std::string& jsonPath,
    ProceduralEffectProfile* outProfile,
    std::string* outError) {
    if (outError) {
        outError->clear();
    }
    if (!outProfile) {
        if (outError) {
            *outError = "invalid_profile_output";
        }
        return false;
    }

    std::string body;
    if (!ReadJsonFile(jsonPath, &body)) {
        if (outError) {
            *outError = "read_profile_file_failed";
        }
        return false;
    }

    json root;
    try {
        root = json::parse(body);
    } catch (...) {
        if (outError) {
            *outError = "parse_profile_json_failed";
        }
        return false;
    }
    if (!root.is_object()) {
        if (outError) {
            *outError = "parse_profile_json_failed";
        }
        return false;
    }

    ProceduralEffectProfile profile{};
    BuildDefaultProfile(&profile);
    if (root.contains("version") && root["version"].is_number_integer()) {
        profile.version = root["version"].get<int>();
    }

    const auto actionsIt = root.find("actions");
    if (actionsIt == root.end() || !actionsIt->is_array()) {
        *outProfile = std::move(profile);
        return true;
    }

    for (const auto& actionNode : *actionsIt) {
        if (!actionNode.is_object()) {
            continue;
        }
        const auto actionNameIt = actionNode.find("action");
        if (actionNameIt == actionNode.end() || !actionNameIt->is_string()) {
            continue;
        }
        PetAction action = PetAction::Idle;
        if (!ParseActionName(actionNameIt->get<std::string>(), &action)) {
            continue;
        }
        ProceduralActionSpec spec{};
        const auto existing = profile.Find(action);
        if (existing) {
            spec = *existing;
        }
        ParseActionSpec(actionNode, &spec);
        profile.actions[static_cast<int>(action)] = spec;
    }

    *outProfile = std::move(profile);
    return true;
}

std::shared_ptr<const ProceduralEffectProfile> CreateDefaultProceduralEffectProfile() {
    ProceduralEffectProfile profile{};
    BuildDefaultProfile(&profile);
    return std::make_shared<const ProceduralEffectProfile>(std::move(profile));
}

} // namespace mousefx::pet
