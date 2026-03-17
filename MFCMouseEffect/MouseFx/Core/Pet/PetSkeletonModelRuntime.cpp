#include "pch.h"
#include "MouseFx/Core/Pet/PetInterfaces.h"

#include "MouseFx/Core/Pet/PetCanonicalModelValidator.h"
#include "MouseFx/Core/Pet/PetGlbSkeletonLoader.h"

#include <algorithm>
#include <cstdint>
#include <unordered_map>
#include <utility>

namespace mousefx::pet {
namespace {

class SkeletonModelRuntime final : public IPetModelRuntime {
public:
    bool LoadCanonicalModel(const CanonicalModelAsset& asset) override {
        loadedAsset_ = asset;
        skeleton_ = {};
        poseByBone_.clear();

        if (asset.canonicalGlbPath.empty()) {
            return false;
        }

        std::string error;
        if (!LoadSkeletonFromGlb(asset.canonicalGlbPath, &skeleton_, &error)) {
            (void)error;
            return false;
        }

        for (size_t i = 0; i < skeleton_.bones.size(); ++i) {
            poseByBone_[skeleton_.bones[i].name] = i;
        }
        resolvedLocalPose_.assign(skeleton_.bones.size(), Transform{});
        resolvedPoseValid_.assign(skeleton_.bones.size(), false);
        filteredPose_.clear();
        loaded_ = true;
        return true;
    }

    void ApplyPose(const SkeletonPose& pose) override {
        if (!loaded_ || skeleton_.bones.empty()) {
            return;
        }
        lastPose_ = pose;

        // Resolve incoming pose to canonical skeleton bone indices.
        std::fill(resolvedPoseValid_.begin(), resolvedPoseValid_.end(), false);
        filteredPose_.clear();
        filteredPose_.reserve(pose.bones.size());
        for (const auto& bonePose : pose.bones) {
            const int32_t resolvedIndex = ResolveBoneIndex(bonePose);
            if (resolvedIndex < 0) {
                continue;
            }
            const size_t index = static_cast<size_t>(resolvedIndex);
            if (index >= resolvedLocalPose_.size()) {
                continue;
            }
            resolvedLocalPose_[index] = bonePose.local;
            resolvedPoseValid_[index] = true;

            BonePose normalized = bonePose;
            normalized.boneIndex = resolvedIndex;
            if (normalized.name.empty()) {
                normalized.name = skeleton_.bones[index].name;
            }
            filteredPose_.push_back(std::move(normalized));
        }
    }

    void ApplyAppearance(const AppearanceOverrides& appearance) override {
        lastAppearance_ = appearance;
    }

    const SkeletonDesc* CurrentSkeleton() const override {
        if (!loaded_) {
            return nullptr;
        }
        return &skeleton_;
    }

private:
    int32_t ResolveBoneIndex(const BonePose& pose) const {
        if (pose.boneIndex >= 0) {
            const size_t direct = static_cast<size_t>(pose.boneIndex);
            if (direct < skeleton_.bones.size()) {
                return pose.boneIndex;
            }
        }
        if (!pose.name.empty()) {
            const auto it = poseByBone_.find(pose.name);
            if (it != poseByBone_.end()) {
                return static_cast<int32_t>(it->second);
            }
        }
        return -1;
    }

    CanonicalModelAsset loadedAsset_{};
    SkeletonDesc skeleton_{};
    std::unordered_map<std::string, size_t> poseByBone_{};
    SkeletonPose lastPose_{};
    std::vector<BonePose> filteredPose_{};
    std::vector<Transform> resolvedLocalPose_{};
    std::vector<uint8_t> resolvedPoseValid_{};
    AppearanceOverrides lastAppearance_{};
    bool loaded_{false};
};

class DefaultModelAssetImporter final : public IModelAssetImporter {
public:
    explicit DefaultModelAssetImporter(std::unique_ptr<IModelFormatConverter> converter)
        : converter_(std::move(converter)) {}

    bool ImportToCanonicalGlb(const std::string& sourcePath, CanonicalModelAsset* outAsset) override {
        if (!outAsset) {
            return false;
        }
        CanonicalModelAsset asset{};
        asset.sourcePath = sourcePath;
        asset.sourceFormat = DetectModelFormatFromPath(sourcePath);
        if (!IsSupportedImportFormat(asset.sourceFormat)) {
            asset.warnings.push_back("unsupported source format");
            *outAsset = std::move(asset);
            return false;
        }

        if (!converter_ || !converter_->Supports(asset.sourceFormat)) {
            asset.warnings.push_back(std::string("no registered converter for format: ") + ToString(asset.sourceFormat));
            *outAsset = std::move(asset);
            return false;
        }

        ModelConversionResult conversion{};
        if (!converter_->ConvertToCanonicalGlb(sourcePath, asset.sourceFormat, &conversion)) {
            asset.warnings.insert(asset.warnings.end(), conversion.warnings.begin(), conversion.warnings.end());
            if (asset.warnings.empty()) {
                asset.warnings.push_back("model conversion failed without diagnostics");
            }
            *outAsset = std::move(asset);
            return false;
        }
        asset.canonicalGlbPath = std::move(conversion.canonicalGlbPath);
        asset.converted = conversion.converted;
        asset.warnings.insert(asset.warnings.end(), conversion.warnings.begin(), conversion.warnings.end());

        CanonicalModelValidationReport report{};
        if (!ValidateCanonicalGlb(asset.canonicalGlbPath, &report)) {
            if (!report.error.empty()) {
                asset.warnings.push_back(std::string("canonical glb validation failed: ") + report.error);
            } else {
                asset.warnings.push_back("canonical glb validation failed");
            }
            *outAsset = std::move(asset);
            return false;
        }
        for (const auto& warning : report.warnings) {
            asset.warnings.push_back(warning);
        }

        *outAsset = std::move(asset);
        return true;
    }

private:
    std::unique_ptr<IModelFormatConverter> converter_{};
};

} // namespace

std::unique_ptr<IPetModelRuntime> CreateDefaultPetModelRuntime() {
    return std::make_unique<SkeletonModelRuntime>();
}

std::unique_ptr<IModelAssetImporter> CreateModelAssetImporter(std::unique_ptr<IModelFormatConverter> converter) {
    if (!converter) {
        return nullptr;
    }
    return std::make_unique<DefaultModelAssetImporter>(std::move(converter));
}

std::unique_ptr<IModelAssetImporter> CreateDefaultModelAssetImporter() {
    return CreateModelAssetImporter(CreateDefaultModelFormatConverter());
}

} // namespace mousefx::pet
