#pragma once

#include <memory>
#include <string>
#include <vector>

#include "MouseFx/Core/Control/MouseCompanionPluginV1Types.h"

namespace mousefx {

struct PetVisualHostUpdate {
    ScreenPoint pt{};
    int actionCode{0};
    float actionIntensity{0.0f};
    float headTintAmount{0.0f};
};

struct PetVisualHostRendererBackendCatalogEntry {
    std::string name;
    int priority{0};
    bool available{false};
    std::string unavailableReason;
    std::vector<std::string> unmetRequirements;
};

struct PetVisualHostDiagnostics {
    std::string preferredRendererBackendSource;
    std::string preferredRendererBackend;
    std::string selectedRendererBackend;
    std::string rendererBackendSelectionReason;
    std::string rendererBackendFailureReason;
    std::vector<std::string> availableRendererBackends;
    std::vector<std::string> unavailableRendererBackends;
    std::vector<PetVisualHostRendererBackendCatalogEntry> rendererBackendCatalog;
};

class IPetVisualHost {
public:
    virtual ~IPetVisualHost() = default;

    virtual bool Start(const MouseCompanionPetRuntimeConfig& config) = 0;
    virtual void Shutdown() = 0;
    virtual bool Configure(const MouseCompanionPetRuntimeConfig& config) = 0;
    virtual bool Show() = 0;
    virtual void Hide() = 0;
    virtual bool LoadModel(const std::string& modelPath) = 0;
    virtual bool LoadActionLibrary(const std::string& actionLibraryPath) = 0;
    virtual bool LoadAppearanceProfile(const std::string& appearanceProfilePath) = 0;
    virtual bool ConfigurePoseBinding(const std::vector<std::string>& boneNames) = 0;
    virtual void MoveFollow(const ScreenPoint& pt) = 0;
    virtual void Update(const PetVisualHostUpdate& update) = 0;
    virtual void ApplyPose(const MouseCompanionPetPoseFrame& poseFrame) = 0;
    virtual bool IsActive() const = 0;
    virtual PetVisualHostDiagnostics ReadDiagnostics() const { return {}; }
};

} // namespace mousefx
