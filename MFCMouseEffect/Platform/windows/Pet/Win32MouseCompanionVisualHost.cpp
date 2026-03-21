#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionVisualHost.h"

#include "MouseFx/Utils/StringUtils.h"
#include "MouseFx/Utils/TimeUtils.h"
#include "Platform/windows/Pet/Win32MouseCompanionActionRuntime.h"
#include "Platform/windows/Pet/Win32MouseCompanionRendererBackendRegistry.h"
#include "Platform/windows/Pet/Win32MouseCompanionRendererBackendPreferenceRequestBuilder.h"
#include "Platform/windows/Pet/Win32MouseCompanionVisualRuntime.h"

namespace mousefx::windows {

bool Win32MouseCompanionVisualHost::Start(const MouseCompanionPetRuntimeConfig& config) {
    actionLibrary_ = {};
    actionRuntime_ = {};
    window_.SetRendererBackendPreferenceRequest(BuildWin32MouseCompanionRendererBackendPreferenceRequest(config));
    ResetWin32MouseCompanionVisualState(&state_, config, config.enabled && window_.Create());
    return state_.active;
}

void Win32MouseCompanionVisualHost::Shutdown() {
    window_.Shutdown();
    state_ = {};
    actionLibrary_ = {};
    actionRuntime_ = {};
    configuredBoneNames_.clear();
}

bool Win32MouseCompanionVisualHost::Configure(const MouseCompanionPetRuntimeConfig& config) {
    window_.SetRendererBackendPreferenceRequest(BuildWin32MouseCompanionRendererBackendPreferenceRequest(config));
    ApplyWin32MouseCompanionVisualConfig(&state_, config);
    if (!state_.active) {
        state_.visible = false;
        window_.Hide();
    } else {
        SyncWindow();
    }
    return state_.active;
}

bool Win32MouseCompanionVisualHost::Show() {
    if (!state_.active) {
        return false;
    }
    state_.visible = window_.Show();
    SyncWindow();
    return state_.visible;
}

void Win32MouseCompanionVisualHost::Hide() {
    window_.Hide();
    state_.visible = false;
}

bool Win32MouseCompanionVisualHost::LoadModel(const std::string& modelPath) {
    const std::string trimmed = TrimAscii(modelPath);
    if (!state_.active || trimmed.empty()) {
        return false;
    }
    ApplyWin32MouseCompanionModelAssetState(&state_, trimmed);
    SyncWindow();
    return false;
}

bool Win32MouseCompanionVisualHost::LoadActionLibrary(const std::string& actionLibraryPath) {
    const std::string trimmed = TrimAscii(actionLibraryPath);
    if (!state_.active || trimmed.empty()) {
        return false;
    }
    LoadWin32MouseCompanionActionRuntimeLibrary(&state_, &actionLibrary_, trimmed);
    RefreshActionRuntime();
    SyncWindow();
    return state_.actionLibraryAvailable;
}

bool Win32MouseCompanionVisualHost::LoadAppearanceProfile(const std::string& appearanceProfilePath) {
    const std::string trimmed = TrimAscii(appearanceProfilePath);
    if (!state_.active || trimmed.empty()) {
        return false;
    }
    Win32MouseCompanionAppearanceProfile profile{};
    const bool loaded = LoadWin32MouseCompanionAppearanceProfileFromPath(trimmed, &profile);
    ApplyWin32MouseCompanionAppearanceState(&state_, std::move(profile), loaded);
    SyncWindow();
    return loaded;
}

bool Win32MouseCompanionVisualHost::ConfigurePoseBinding(const std::vector<std::string>& boneNames) {
    configuredBoneNames_ = boneNames;
    ApplyWin32MouseCompanionPoseBindingState(&state_, !configuredBoneNames_.empty());
    SyncWindow();
    return false;
}

void Win32MouseCompanionVisualHost::MoveFollow(const ScreenPoint& pt) {
    ApplyWin32MouseCompanionFollowPoint(&state_, pt);
    SyncWindow();
}

void Win32MouseCompanionVisualHost::Update(const PetVisualHostUpdate& update) {
    ApplyWin32MouseCompanionHostUpdate(&state_, update);
    if (!state_.active) {
        return;
    }
    UpdateActionRuntimeSelection(update.actionCode == 2 || update.actionCode == 5);
    RefreshActionRuntime();
    SyncWindow();
}

void Win32MouseCompanionVisualHost::ApplyPose(const MouseCompanionPetPoseFrame& poseFrame) {
    ApplyWin32MouseCompanionPoseFrame(&state_, poseFrame);
    if (!state_.active) {
        return;
    }
    UpdateActionRuntimeSelection(false);
    RefreshActionRuntime();
    SyncWindow();
}

bool Win32MouseCompanionVisualHost::IsActive() const {
    return state_.active;
}

PetVisualHostDiagnostics Win32MouseCompanionVisualHost::ReadDiagnostics() const {
    PetVisualHostDiagnostics diagnostics{};
    diagnostics.preferredRendererBackendSource = window_.PreferredRendererBackendSource();
    diagnostics.preferredRendererBackend = window_.PreferredRendererBackendName();
    diagnostics.selectedRendererBackend = window_.SelectedRendererBackendName();
    diagnostics.rendererBackendSelectionReason = window_.RendererBackendSelectionReason();
    diagnostics.rendererBackendFailureReason = window_.RendererBackendFailureReason();
    diagnostics.availableRendererBackends = window_.AvailableRendererBackendNames();
    diagnostics.unavailableRendererBackends = window_.UnavailableRendererBackendNames();
    for (const auto& descriptor : Win32MouseCompanionRendererBackendRegistry::Instance().ListByPriority()) {
        PetVisualHostRendererBackendCatalogEntry entry{};
        entry.name = descriptor.name;
        entry.priority = descriptor.priority;
        entry.available = descriptor.available;
        entry.unavailableReason = descriptor.unavailableReason;
        entry.unmetRequirements = descriptor.unmetRequirements;
        diagnostics.rendererBackendCatalog.push_back(std::move(entry));
    }
    return diagnostics;
}

void Win32MouseCompanionVisualHost::SyncWindow() {
    if (!state_.active || !state_.visible) {
        return;
    }
    window_.Update(state_);
}

void Win32MouseCompanionVisualHost::RefreshActionRuntime() {
    RefreshWin32MouseCompanionActionRuntimeSample(&state_, actionLibrary_, actionRuntime_, NowMs());
}

void Win32MouseCompanionVisualHost::UpdateActionRuntimeSelection(bool restartOneShot) {
    UpdateWin32MouseCompanionActionRuntimeSelection(state_, restartOneShot, NowMs(), &actionRuntime_);
}

} // namespace mousefx::windows
