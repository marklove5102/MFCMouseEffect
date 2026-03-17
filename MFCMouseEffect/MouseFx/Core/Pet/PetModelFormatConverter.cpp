#include "pch.h"
#include "MouseFx/Core/Pet/PetInterfaces.h"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <utility>

namespace mousefx::pet {
std::unique_ptr<IModelFormatConverter> CreateGltfJsonCanonicalFormatConverter();
std::unique_ptr<IModelFormatConverter> CreateToolBackedUsdzCanonicalFormatConverter();
std::unique_ptr<IModelFormatConverter> CreateToolBackedFbxCanonicalFormatConverter();

namespace {

constexpr uint32_t kGlbMagic = 0x46546C67u; // "glTF"
constexpr uint32_t kGlbVersion2 = 2u;

uint32_t ReadU32Le(const uint8_t* p) {
    return static_cast<uint32_t>(p[0]) |
           (static_cast<uint32_t>(p[1]) << 8) |
           (static_cast<uint32_t>(p[2]) << 16) |
           (static_cast<uint32_t>(p[3]) << 24);
}

bool IsBinaryGlbV2(const std::filesystem::path& path, std::string* outError) {
    if (outError) {
        outError->clear();
    }

    std::ifstream in(path, std::ios::binary);
    if (!in) {
        if (outError) {
            *outError = "failed to open source model";
        }
        return false;
    }

    uint8_t header[12] = {0};
    in.read(reinterpret_cast<char*>(header), static_cast<std::streamsize>(sizeof(header)));
    if (in.gcount() != static_cast<std::streamsize>(sizeof(header))) {
        if (outError) {
            *outError = "file too small for glb header";
        }
        return false;
    }

    const uint32_t magic = ReadU32Le(header);
    const uint32_t version = ReadU32Le(header + 4);
    const uint32_t declaredLength = ReadU32Le(header + 8);

    if (magic != kGlbMagic) {
        if (outError) {
            *outError = "invalid glb magic";
        }
        return false;
    }
    if (version != kGlbVersion2) {
        if (outError) {
            *outError = "unsupported glb version";
        }
        return false;
    }

    std::error_code ec;
    const uintmax_t fileSize = std::filesystem::file_size(path, ec);
    if (ec || fileSize < 12 || declaredLength > fileSize) {
        if (outError) {
            *outError = "invalid glb declared length";
        }
        return false;
    }

    return true;
}

void AppendWarnings(const std::vector<std::string>& source, std::vector<std::string>* target) {
    if (!target) {
        return;
    }
    target->insert(target->end(), source.begin(), source.end());
}

class GlbPassthroughFormatConverter final : public IModelFormatConverter {
public:
    bool Supports(ModelFormat sourceFormat) const override {
        return sourceFormat == ModelFormat::Glb;
    }

    bool ConvertToCanonicalGlb(const std::string& sourcePath,
                               ModelFormat sourceFormat,
                               ModelConversionResult* outResult) override {
        if (!outResult) {
            return false;
        }
        *outResult = {};

        if (!Supports(sourceFormat)) {
            return false;
        }
        if (sourcePath.empty()) {
            outResult->warnings.push_back("empty source path");
            return false;
        }

        const std::filesystem::path source = std::filesystem::path(sourcePath).lexically_normal();
        if (!std::filesystem::exists(source)) {
            outResult->warnings.push_back("source file does not exist");
            return false;
        }

        outResult->canonicalGlbPath = source.string();
        outResult->converted = false;
        return true;
    }
};

class VrmBinaryCanonicalFormatConverter final : public IModelFormatConverter {
public:
    bool Supports(ModelFormat sourceFormat) const override {
        return sourceFormat == ModelFormat::Vrm;
    }

    bool ConvertToCanonicalGlb(const std::string& sourcePath,
                               ModelFormat sourceFormat,
                               ModelConversionResult* outResult) override {
        if (!outResult) {
            return false;
        }
        *outResult = {};

        if (!Supports(sourceFormat)) {
            return false;
        }
        if (sourcePath.empty()) {
            outResult->warnings.push_back("converter.vrm.empty_source_path");
            return false;
        }

        const std::filesystem::path source = std::filesystem::path(sourcePath).lexically_normal();
        if (!std::filesystem::exists(source)) {
            outResult->warnings.push_back("converter.vrm.source_missing");
            return false;
        }

        std::string glbProbeError;
        if (!IsBinaryGlbV2(source, &glbProbeError)) {
            outResult->warnings.push_back(std::string("converter.vrm.invalid_binary_glb: ") + glbProbeError);
            return false;
        }

        const std::filesystem::path sourceDir = source.parent_path();
        const std::filesystem::path canonicalDir = sourceDir / "canonical";
        const std::filesystem::path canonicalPath = canonicalDir / (source.stem().string() + ".glb");

        std::error_code ec;
        std::filesystem::create_directories(canonicalDir, ec);
        if (ec) {
            outResult->canonicalGlbPath = source.string();
            outResult->converted = false;
            outResult->warnings.push_back(std::string("converter.vrm.canonical_dir_unavailable_using_source: ") + ec.message());
            return true;
        }

        const bool canonicalExists = std::filesystem::exists(canonicalPath, ec) && !ec;
        if (canonicalExists) {
            const auto sourceTime = std::filesystem::last_write_time(source, ec);
            const bool sourceTimeOk = !ec;
            ec.clear();
            const auto targetTime = std::filesystem::last_write_time(canonicalPath, ec);
            const bool targetTimeOk = !ec;
            ec.clear();
            const uintmax_t sourceSize = std::filesystem::file_size(source, ec);
            const bool sourceSizeOk = !ec;
            ec.clear();
            const uintmax_t targetSize = std::filesystem::file_size(canonicalPath, ec);
            const bool targetSizeOk = !ec;
            ec.clear();

            if (sourceTimeOk && targetTimeOk && sourceSizeOk && targetSizeOk &&
                sourceSize == targetSize && targetTime >= sourceTime) {
                outResult->canonicalGlbPath = canonicalPath.string();
                outResult->converted = true;
                outResult->warnings.push_back(std::string("converter.vrm.reuse_cached_canonical: ") +
                                              canonicalPath.filename().string());
                return true;
            }
        }

        std::filesystem::copy_file(source, canonicalPath, std::filesystem::copy_options::overwrite_existing, ec);
        if (ec) {
            outResult->canonicalGlbPath = source.string();
            outResult->converted = false;
            outResult->warnings.push_back(std::string("converter.vrm.copy_failed_using_source: ") + ec.message());
            return true;
        }

        outResult->canonicalGlbPath = canonicalPath.string();
        outResult->converted = true;
        outResult->warnings.push_back(std::string("converter.vrm.exported_canonical: ") +
                                      canonicalPath.filename().string());
        return true;
    }
};

class SidecarCanonicalFormatConverter final : public IModelFormatConverter {
public:
    bool Supports(ModelFormat sourceFormat) const override {
        switch (sourceFormat) {
        case ModelFormat::Gltf:
        case ModelFormat::Usdz:
        case ModelFormat::Vrm:
        case ModelFormat::Fbx:
            return true;
        default:
            return false;
        }
    }

    bool ConvertToCanonicalGlb(const std::string& sourcePath,
                               ModelFormat sourceFormat,
                               ModelConversionResult* outResult) override {
        if (!outResult) {
            return false;
        }
        *outResult = {};

        if (!Supports(sourceFormat)) {
            return false;
        }
        if (sourcePath.empty()) {
            outResult->warnings.push_back("empty source path");
            return false;
        }

        const std::filesystem::path source = std::filesystem::path(sourcePath).lexically_normal();
        if (!std::filesystem::exists(source)) {
            outResult->warnings.push_back("source file does not exist");
            return false;
        }

        const std::filesystem::path sourceDir = source.parent_path();
        const std::string stem = source.stem().string();
        const std::filesystem::path sidecarSameDir = sourceDir / (stem + ".glb");
        const std::filesystem::path sidecarCanonicalDir = sourceDir / "canonical" / (stem + ".glb");
        const std::filesystem::path sidecarCanonicalSuffix = sourceDir / (stem + ".canonical.glb");

        const std::filesystem::path candidates[] = {
            sidecarSameDir,
            sidecarCanonicalDir,
            sidecarCanonicalSuffix,
        };
        for (const auto& candidate : candidates) {
            if (!std::filesystem::exists(candidate)) {
                continue;
            }
            outResult->canonicalGlbPath = candidate.lexically_normal().string();
            outResult->converted = true;
            outResult->warnings.push_back(std::string("resolved preconverted canonical glb: ") +
                                          candidate.filename().string());
            return true;
        }

        outResult->warnings.push_back(std::string("no converter backend registered for format '") +
                                      ToString(sourceFormat) +
                                      "'; provide canonical .glb sidecar");
        return false;
    }
};

class CompositeModelFormatConverter final : public IModelFormatConverter {
public:
    explicit CompositeModelFormatConverter(std::vector<std::unique_ptr<IModelFormatConverter>> converters)
        : converters_(std::move(converters)) {}

    bool Supports(ModelFormat sourceFormat) const override {
        for (const auto& converter : converters_) {
            if (converter && converter->Supports(sourceFormat)) {
                return true;
            }
        }
        return false;
    }

    bool ConvertToCanonicalGlb(const std::string& sourcePath,
                               ModelFormat sourceFormat,
                               ModelConversionResult* outResult) override {
        if (!outResult) {
            return false;
        }
        *outResult = {};

        bool hasSupportedConverter = false;
        for (const auto& converter : converters_) {
            if (!converter || !converter->Supports(sourceFormat)) {
                continue;
            }
            hasSupportedConverter = true;

            ModelConversionResult attempt{};
            if (converter->ConvertToCanonicalGlb(sourcePath, sourceFormat, &attempt)) {
                *outResult = std::move(attempt);
                return true;
            }
            AppendWarnings(attempt.warnings, &outResult->warnings);
        }

        if (!hasSupportedConverter) {
            outResult->warnings.push_back(std::string("no converter registered for format: ") +
                                          ToString(sourceFormat));
        }
        return false;
    }

private:
    std::vector<std::unique_ptr<IModelFormatConverter>> converters_{};
};

} // namespace

std::unique_ptr<IModelFormatConverter> CreateDefaultModelFormatConverter() {
    std::vector<std::unique_ptr<IModelFormatConverter>> converters{};
    converters.push_back(std::make_unique<GlbPassthroughFormatConverter>());
    converters.push_back(std::make_unique<VrmBinaryCanonicalFormatConverter>());
    converters.push_back(CreateGltfJsonCanonicalFormatConverter());
    converters.push_back(CreateToolBackedUsdzCanonicalFormatConverter());
    converters.push_back(CreateToolBackedFbxCanonicalFormatConverter());
    converters.push_back(std::make_unique<SidecarCanonicalFormatConverter>());
    return std::make_unique<CompositeModelFormatConverter>(std::move(converters));
}

} // namespace mousefx::pet
