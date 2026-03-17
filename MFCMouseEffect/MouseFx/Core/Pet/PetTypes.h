#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace mousefx::pet {

struct Vec3 final {
    float x{0.0f};
    float y{0.0f};
    float z{0.0f};
};

struct Quat final {
    float x{0.0f};
    float y{0.0f};
    float z{0.0f};
    float w{1.0f};
};

struct Transform final {
    Vec3 position{};
    Quat rotation{};
    Vec3 scale{1.0f, 1.0f, 1.0f};
};

struct BonePose final {
    int32_t boneIndex{-1};
    std::string name;
    Transform local{};
};

struct SkeletonBone final {
    std::string name;
    int32_t parentIndex{-1};
    int32_t sourceNodeIndex{-1};
};

struct SkeletonDesc final {
    std::string name{};
    std::vector<SkeletonBone> bones{};
};

struct SkeletonPose final {
    std::vector<BonePose> bones{};
    float locomotionForward{0.0f};
    float locomotionTurn{0.0f};
    float actionIntensity{0.0f};
};

enum class PetAction : uint8_t {
    Idle = 0,
    Follow = 1,
    ClickReact = 2,
    Drag = 3,
};

struct PetFrameInput final {
    double dtSeconds{0.0};
    bool cursorVisible{true};
    Vec3 cursorPosition{};
    bool primaryPressed{false};
    bool dragging{false};
};

struct AppearanceOverrides final {
    std::string skinVariantId{};
    std::vector<std::string> enabledAccessoryIds{};
    std::vector<std::string> textureOverridePaths{};
};

} // namespace mousefx::pet
