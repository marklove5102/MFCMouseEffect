#include "pch.h"
#include "MouseFx/Core/Pet/PetInterfaces.h"

namespace mousefx::pet {
namespace {

class NullPetModelRuntime final : public IPetModelRuntime {
public:
    bool LoadCanonicalModel(const CanonicalModelAsset& asset) override {
        (void)asset;
        return true;
    }

    void ApplyPose(const SkeletonPose& pose) override {
        (void)pose;
    }

    void ApplyAppearance(const AppearanceOverrides& appearance) override {
        (void)appearance;
    }

    const SkeletonDesc* CurrentSkeleton() const override {
        return nullptr;
    }
};

} // namespace

std::unique_ptr<IPetModelRuntime> CreateNullPetModelRuntime() {
    return std::make_unique<NullPetModelRuntime>();
}

} // namespace mousefx::pet
