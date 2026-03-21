#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionActionRuntime.h"

#include "MouseFx/Utils/StringUtils.h"

#include <filesystem>

namespace mousefx::windows {

bool LoadWin32MouseCompanionActionRuntimeLibrary(
    Win32MouseCompanionVisualState* state,
    Win32MouseCompanionActionLibrary* library,
    const std::string& actionLibraryPath) {
    if (!state || !library || !state->active || actionLibraryPath.empty()) {
        return false;
    }
    state->lastActionLibraryPath = actionLibraryPath;
    state->actionLibraryAvailable =
        std::filesystem::exists(std::filesystem::path(actionLibraryPath)) &&
        LoadWin32MouseCompanionActionLibraryFromPath(actionLibraryPath, library);
    return state->actionLibraryAvailable;
}

void UpdateWin32MouseCompanionActionRuntimeSelection(
    const Win32MouseCompanionVisualState& visualState,
    bool restartOneShot,
    uint64_t nowMs,
    Win32MouseCompanionActionRuntimeState* runtimeState) {
    if (!runtimeState) {
        return;
    }
    const std::string nextActionKey = ToLowerAscii(TrimAscii(visualState.lastActionName));
    if (runtimeState->activeActionKey != nextActionKey ||
        restartOneShot ||
        runtimeState->actionClipStartTickMs == 0) {
        runtimeState->activeActionKey = nextActionKey;
        runtimeState->actionClipStartTickMs = nowMs;
    }
}

void RefreshWin32MouseCompanionActionRuntimeSample(
    Win32MouseCompanionVisualState* state,
    const Win32MouseCompanionActionLibrary& library,
    const Win32MouseCompanionActionRuntimeState& runtimeState,
    uint64_t nowMs) {
    if (!state) {
        return;
    }
    state->latestActionClipSample = {};
    if (!state->actionLibraryAvailable || runtimeState.actionClipStartTickMs == 0) {
        return;
    }
    const uint64_t elapsedMs =
        (nowMs >= runtimeState.actionClipStartTickMs) ? (nowMs - runtimeState.actionClipStartTickMs) : 0;
    SampleWin32MouseCompanionActionLibrary(
        library,
        state->lastActionName,
        elapsedMs,
        &state->latestActionClipSample);
}

} // namespace mousefx::windows
