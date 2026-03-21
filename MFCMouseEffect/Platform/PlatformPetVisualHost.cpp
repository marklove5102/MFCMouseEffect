#include "pch.h"

#include "Platform/PlatformPetVisualHost.h"

#include "MouseFx/Core/Control/IPetVisualHost.h"
#include "MouseFx/Utils/StringUtils.h"
#include "Platform/PlatformTarget.h"

#if MFX_PLATFORM_MACOS
#include "Platform/macos/Pet/MacosMouseCompanionPhase1SwiftBridge.h"
#endif

#if MFX_PLATFORM_WINDOWS
#include "Platform/windows/Pet/Win32MouseCompanionVisualHost.h"
#endif

#include <vector>

namespace mousefx::platform {
namespace {

#if MFX_PLATFORM_MACOS
class MacosPetVisualHost final : public IPetVisualHost {
public:
    bool Start(const MouseCompanionPetRuntimeConfig& config) override {
        if (handle_) {
            return true;
        }
        handle_ = mfx_macos_mouse_companion_panel_create_v1(
            config.sizePx,
            config.offsetX,
            config.offsetY);
        return handle_ != nullptr;
    }

    void Shutdown() override {
        if (!handle_) {
            return;
        }
        mfx_macos_mouse_companion_panel_hide_v1(handle_);
        mfx_macos_mouse_companion_panel_release_v1(handle_);
        handle_ = nullptr;
    }

    bool Configure(const MouseCompanionPetRuntimeConfig& config) override {
        if (!handle_) {
            return false;
        }
        mfx_macos_mouse_companion_panel_configure_v1(
            handle_,
            config.sizePx,
            ResolvePositionModeCode(config.positionMode),
            ResolveEdgeClampModeCode(config.edgeClampMode),
            config.offsetX,
            config.offsetY,
            config.absoluteX,
            config.absoluteY,
            config.targetMonitor.c_str());
        return true;
    }

    bool Show() override {
        if (!handle_) {
            return false;
        }
        mfx_macos_mouse_companion_panel_show_v1(handle_);
        return true;
    }

    void Hide() override {
        if (handle_) {
            mfx_macos_mouse_companion_panel_hide_v1(handle_);
        }
    }

    bool LoadModel(const std::string& modelPath) override {
        return handle_ && !modelPath.empty() &&
            mfx_macos_mouse_companion_panel_load_model_v1(handle_, modelPath.c_str());
    }

    bool LoadActionLibrary(const std::string& actionLibraryPath) override {
        return handle_ && !actionLibraryPath.empty() &&
            mfx_macos_mouse_companion_panel_load_action_library_v1(handle_, actionLibraryPath.c_str());
    }

    bool LoadAppearanceProfile(const std::string& appearanceProfilePath) override {
        (void)appearanceProfilePath;
        return false;
    }

    bool ConfigurePoseBinding(const std::vector<std::string>& boneNames) override {
        if (!handle_ || boneNames.empty()) {
            return false;
        }
        std::vector<const char*> ptrs;
        ptrs.reserve(boneNames.size());
        for (const std::string& name : boneNames) {
            ptrs.push_back(name.c_str());
        }
        return mfx_macos_mouse_companion_panel_configure_pose_binding_v1(
            handle_,
            ptrs.data(),
            static_cast<int32_t>(ptrs.size()));
    }

    void MoveFollow(const ScreenPoint& pt) override {
        if (handle_) {
            mfx_macos_mouse_companion_panel_move_follow_v1(handle_, pt.x, pt.y);
        }
    }

    void Update(const PetVisualHostUpdate& update) override {
        if (handle_) {
            mfx_macos_mouse_companion_panel_update_v1(
                handle_,
                update.actionCode,
                update.actionIntensity,
                update.headTintAmount);
        }
    }

    void ApplyPose(const MouseCompanionPetPoseFrame& poseFrame) override {
        if (!handle_ || poseFrame.samples.empty()) {
            return;
        }
        std::vector<int32_t> boneIndices;
        std::vector<float> positions;
        std::vector<float> rotations;
        std::vector<float> scales;
        boneIndices.reserve(poseFrame.samples.size());
        positions.reserve(poseFrame.samples.size() * 3);
        rotations.reserve(poseFrame.samples.size() * 4);
        scales.reserve(poseFrame.samples.size() * 3);
        for (const auto& sample : poseFrame.samples) {
            boneIndices.push_back(sample.boneIndex);
            positions.insert(positions.end(), std::begin(sample.position), std::end(sample.position));
            rotations.insert(rotations.end(), std::begin(sample.rotation), std::end(sample.rotation));
            scales.insert(scales.end(), std::begin(sample.scale), std::end(sample.scale));
        }
        mfx_macos_mouse_companion_panel_apply_pose_v1(
            handle_,
            boneIndices.data(),
            positions.data(),
            rotations.data(),
            scales.data(),
            static_cast<int32_t>(poseFrame.samples.size()));
    }

    bool IsActive() const override {
        return handle_ != nullptr;
    }

    PetVisualHostDiagnostics ReadDiagnostics() const override {
        return {};
    }

private:
    static int ResolveEdgeClampModeCode(const std::string& mode) {
        const std::string normalized = ToLowerAscii(TrimAscii(mode));
        if (normalized == "strict") {
            return 0;
        }
        if (normalized == "free") {
            return 2;
        }
        return 1;
    }

    static int ResolvePositionModeCode(const std::string& mode) {
        const std::string normalized = ToLowerAscii(TrimAscii(mode));
        if (normalized == "relative" || normalized == "follow") {
            return 0;
        }
        if (normalized == "absolute") {
            return 1;
        }
        return 2;
    }

    void* handle_{nullptr};
};
#endif

} // namespace

std::unique_ptr<IPetVisualHost> CreatePetVisualHost() {
#if MFX_PLATFORM_MACOS
    return std::make_unique<MacosPetVisualHost>();
#elif MFX_PLATFORM_WINDOWS
    return std::make_unique<windows::Win32MouseCompanionVisualHost>();
#else
    return {};
#endif
}

} // namespace mousefx::platform
