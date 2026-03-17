#include "pch.h"
#include "MouseFx/Core/Pet/PetCompanionRuntime.h"

namespace mousefx::pet {

PetCompanionRuntime::PetCompanionRuntime(std::unique_ptr<IPetModelRuntime> modelRuntime,
                                         std::unique_ptr<IActionSynthesizer> actionSynthesizer)
    : modelRuntime_(std::move(modelRuntime)),
      actionSynthesizer_(std::move(actionSynthesizer)) {}

bool PetCompanionRuntime::LoadCanonicalModel(const CanonicalModelAsset& asset) {
    if (!modelRuntime_) {
        return false;
    }
    const bool loaded = modelRuntime_->LoadCanonicalModel(asset);
    if (!loaded) {
        return false;
    }
    if (actionSynthesizer_) {
        actionSynthesizer_->SetActionLibrary(actionLibrary_);
        actionSynthesizer_->BindSkeleton(modelRuntime_->CurrentSkeleton());
    }
    return true;
}

bool PetCompanionRuntime::LoadActionLibraryFromJson(const std::string& jsonPath) {
    ActionLibrary library{};
    std::string error;
    if (!LoadActionLibraryFromJsonFile(jsonPath, &library, &error)) {
        (void)error;
        return false;
    }

    actionLibrary_ = std::make_shared<ActionLibrary>(std::move(library));
    if (actionSynthesizer_) {
        actionSynthesizer_->SetActionLibrary(actionLibrary_);
        if (modelRuntime_) {
            actionSynthesizer_->BindSkeleton(modelRuntime_->CurrentSkeleton());
        }
    }
    return true;
}

void PetCompanionRuntime::Tick(const PetFrameInput& input) {
    if (!modelRuntime_ || !actionSynthesizer_) {
        return;
    }
    actionSynthesizer_->Update(input, &workingPose_);
    modelRuntime_->ApplyPose(workingPose_);
}

void PetCompanionRuntime::RequestAction(PetAction action, double blendDurationSeconds) {
    if (!actionSynthesizer_) {
        return;
    }
    actionSynthesizer_->RequestAction(action, blendDurationSeconds);
}

void PetCompanionRuntime::ApplyAppearance(const AppearanceOverrides& appearance) {
    currentAppearance_ = appearance;
    if (!modelRuntime_) {
        return;
    }
    modelRuntime_->ApplyAppearance(appearance);
}

const AppearanceOverrides& PetCompanionRuntime::CurrentAppearance() const {
    return currentAppearance_;
}

const SkeletonDesc* PetCompanionRuntime::CurrentSkeleton() const {
    if (!modelRuntime_) {
        return nullptr;
    }
    return modelRuntime_->CurrentSkeleton();
}

const SkeletonPose& PetCompanionRuntime::LastPose() const {
    return workingPose_;
}

} // namespace mousefx::pet
