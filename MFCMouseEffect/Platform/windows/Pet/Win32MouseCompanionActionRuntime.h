#pragma once

#include <cstdint>
#include <string>

#include "Platform/windows/Pet/Win32MouseCompanionActionLibrary.h"
#include "Platform/windows/Pet/Win32MouseCompanionVisualState.h"

namespace mousefx::windows {

struct Win32MouseCompanionActionRuntimeState {
    std::string activeActionKey;
    uint64_t actionClipStartTickMs{0};
};

bool LoadWin32MouseCompanionActionRuntimeLibrary(
    Win32MouseCompanionVisualState* state,
    Win32MouseCompanionActionLibrary* library,
    const std::string& actionLibraryPath);

void UpdateWin32MouseCompanionActionRuntimeSelection(
    const Win32MouseCompanionVisualState& visualState,
    bool restartOneShot,
    uint64_t nowMs,
    Win32MouseCompanionActionRuntimeState* runtimeState);

void RefreshWin32MouseCompanionActionRuntimeSample(
    Win32MouseCompanionVisualState* state,
    const Win32MouseCompanionActionLibrary& library,
    const Win32MouseCompanionActionRuntimeState& runtimeState,
    uint64_t nowMs);

} // namespace mousefx::windows
