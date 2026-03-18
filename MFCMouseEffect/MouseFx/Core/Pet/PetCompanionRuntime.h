#pragma once

#include <memory>
#include <string>

#include "MouseFx/Core/Pet/PetInterfaces.h"

namespace mousefx::pet {

class PetCompanionRuntime final : public IPetController {
public:
    PetCompanionRuntime(std::unique_ptr<IPetModelRuntime> modelRuntime,
                        std::unique_ptr<IActionSynthesizer> actionSynthesizer);

    void Tick(const PetFrameInput& input) override;
    bool LoadCanonicalModel(const CanonicalModelAsset& asset);
    bool LoadActionLibraryFromJson(const std::string& jsonPath);
    bool LoadEffectProfileFromJson(const std::string& jsonPath);

    void RequestAction(PetAction action, double blendDurationSeconds);
    void ApplyAppearance(const AppearanceOverrides& appearance);
    const AppearanceOverrides& CurrentAppearance() const;
    const SkeletonDesc* CurrentSkeleton() const;
    const SkeletonPose& LastPose() const;

private:
    std::unique_ptr<IPetModelRuntime> modelRuntime_{};
    std::unique_ptr<IActionSynthesizer> actionSynthesizer_{};
    std::shared_ptr<const ActionLibrary> actionLibrary_{};
    std::shared_ptr<const ProceduralEffectProfile> effectProfile_{};
    AppearanceOverrides currentAppearance_{};
    SkeletonPose workingPose_{};
};

} // namespace mousefx::pet
