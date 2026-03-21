#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace mousefx::windows {

struct Win32MouseCompanionActionKeyframe final {
    float t{0.0f};
    bool hasRotation{false};
    float rotation[4]{0.0f, 0.0f, 0.0f, 1.0f};
    bool hasScale{false};
    float scale[3]{1.0f, 1.0f, 1.0f};
};

struct Win32MouseCompanionActionTrack final {
    std::string bone;
    std::vector<Win32MouseCompanionActionKeyframe> keyframes;
};

struct Win32MouseCompanionActionClip final {
    std::string action;
    float durationSec{0.0f};
    bool loop{false};
    std::vector<Win32MouseCompanionActionTrack> tracks;
};

struct Win32MouseCompanionActionLibrary final {
    std::unordered_map<std::string, Win32MouseCompanionActionClip> clipsByAction;
};

struct Win32MouseCompanionActionSample final {
    bool valid{false};
    float bodyScaleX{1.0f};
    float bodyScaleY{1.0f};
    float bodyLean{0.0f};
    float headPitch{0.0f};
    float headYaw{0.0f};
};

bool LoadWin32MouseCompanionActionLibraryFromPath(
    const std::string& path,
    Win32MouseCompanionActionLibrary* outLibrary);

bool SampleWin32MouseCompanionActionLibrary(
    const Win32MouseCompanionActionLibrary& library,
    const std::string& actionName,
    uint64_t actionElapsedMs,
    Win32MouseCompanionActionSample* outSample);

} // namespace mousefx::windows
