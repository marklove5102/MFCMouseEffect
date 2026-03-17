#pragma once

#include <memory>
#include <string>
#include <vector>

#include "MouseFx/Core/Pet/PetActionLibrary.h"
#include "MouseFx/Core/Pet/PetAssetFormat.h"
#include "MouseFx/Core/Pet/PetTypes.h"

namespace mousefx::pet {

struct CanonicalModelAsset final {
    std::string sourcePath{};
    std::string canonicalGlbPath{};
    ModelFormat sourceFormat{ModelFormat::Unknown};
    bool converted{false};
    std::vector<std::string> warnings{};
};

struct ModelConversionResult final {
    std::string canonicalGlbPath{};
    bool converted{false};
    std::vector<std::string> warnings{};
};

class IModelFormatConverter {
public:
    virtual ~IModelFormatConverter() = default;
    virtual bool Supports(ModelFormat sourceFormat) const = 0;
    virtual bool ConvertToCanonicalGlb(const std::string& sourcePath,
                                       ModelFormat sourceFormat,
                                       ModelConversionResult* outResult) = 0;
};

class IModelAssetImporter {
public:
    virtual ~IModelAssetImporter() = default;
    virtual bool ImportToCanonicalGlb(const std::string& sourcePath, CanonicalModelAsset* outAsset) = 0;
};

class IPetModelRuntime {
public:
    virtual ~IPetModelRuntime() = default;
    virtual bool LoadCanonicalModel(const CanonicalModelAsset& asset) = 0;
    virtual void ApplyPose(const SkeletonPose& pose) = 0;
    virtual void ApplyAppearance(const AppearanceOverrides& appearance) = 0;
    virtual const SkeletonDesc* CurrentSkeleton() const = 0;
};

class IActionSynthesizer {
public:
    virtual ~IActionSynthesizer() = default;
    virtual void BindSkeleton(const SkeletonDesc* skeleton) = 0;
    virtual void SetActionLibrary(std::shared_ptr<const ActionLibrary> actionLibrary) = 0;
    virtual void RequestAction(PetAction action, double blendDurationSeconds) = 0;
    virtual void Update(const PetFrameInput& input, SkeletonPose* outPose) = 0;
};

class IPetController {
public:
    virtual ~IPetController() = default;
    virtual void Tick(const PetFrameInput& input) = 0;
};

std::unique_ptr<IActionSynthesizer> CreateDefaultActionSynthesizer();
std::unique_ptr<IPetModelRuntime> CreateNullPetModelRuntime();
std::unique_ptr<IPetModelRuntime> CreateDefaultPetModelRuntime();
std::unique_ptr<IModelFormatConverter> CreateDefaultModelFormatConverter();
std::unique_ptr<IModelAssetImporter> CreateModelAssetImporter(std::unique_ptr<IModelFormatConverter> converter);
std::unique_ptr<IModelAssetImporter> CreateDefaultModelAssetImporter();

} // namespace mousefx::pet
