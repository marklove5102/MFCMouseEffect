#pragma once

#include <vector>

#include "MouseFx/Core/Control/IPetVisualHost.h"
#include "Platform/windows/Pet/Win32MouseCompanionActionLibrary.h"
#include "Platform/windows/Pet/Win32MouseCompanionActionRuntime.h"
#include "Platform/windows/Pet/Win32MouseCompanionAppearanceProfile.h"
#include "Platform/windows/Pet/Win32MouseCompanionWindow.h"
#include "Platform/windows/Pet/Win32MouseCompanionVisualState.h"

namespace mousefx::windows {

class Win32MouseCompanionVisualHost final : public IPetVisualHost {
public:
    Win32MouseCompanionVisualHost() = default;
    ~Win32MouseCompanionVisualHost() override = default;

    bool Start(const MouseCompanionPetRuntimeConfig& config) override;
    void Shutdown() override;
    bool Configure(const MouseCompanionPetRuntimeConfig& config) override;
    bool Show() override;
    void Hide() override;
    bool LoadModel(const std::string& modelPath) override;
    bool LoadActionLibrary(const std::string& actionLibraryPath) override;
    bool LoadAppearanceProfile(const std::string& appearanceProfilePath) override;
    bool ConfigurePoseBinding(const std::vector<std::string>& boneNames) override;
    void MoveFollow(const ScreenPoint& pt) override;
    void Update(const PetVisualHostUpdate& update) override;
    void ApplyPose(const MouseCompanionPetPoseFrame& poseFrame) override;
    bool IsActive() const override;
    PetVisualHostDiagnostics ReadDiagnostics() const override;

private:
    void SyncWindow();
    void RefreshActionRuntime();
    void UpdateActionRuntimeSelection(bool restartOneShot);

    Win32MouseCompanionVisualState state_{};
    Win32MouseCompanionWindow window_{};
    Win32MouseCompanionActionLibrary actionLibrary_{};
    Win32MouseCompanionActionRuntimeState actionRuntime_{};
    Win32MouseCompanionRendererBackendPreferenceRequest configuredRendererBackendPreferenceRequest_{};
    std::vector<std::string> configuredBoneNames_{};
};

} // namespace mousefx::windows
