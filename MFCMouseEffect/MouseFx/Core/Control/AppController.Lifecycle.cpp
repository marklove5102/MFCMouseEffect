#include "pch.h"

#include "AppController.h"

#include "MouseFx/Core/Config/ConfigPathResolver.h"
#include "MouseFx/Core/Config/EffectConfigInternal.h"
#include "MouseFx/Core/Overlay/OverlayHostService.h"
#include "MouseFx/Core/Pet/PetActionCoverageAnalyzer.h"
#include "MouseFx/Core/Pet/PetAppearanceProfile.h"
#include "MouseFx/Core/Pet/PetCompanionRuntime.h"
#include "MouseFx/Core/Pet/PetInterfaces.h"
#include "MouseFx/Core/System/GdiPlusSession.h"
#include "MouseFx/Renderers/Hold/Presentation/QuantumHaloPresenterSelection.h"
#include "MouseFx/Styles/ThemeStyle.h"
#include "MouseFx/Utils/StringUtils.h"
#include "Platform/PlatformTarget.h"
#if MFX_PLATFORM_MACOS
#include "Platform/macos/Pet/MacosMouseCompanionSwiftBridge.h"
#endif

#include <filesystem>
#include <vector>

namespace mousefx {
namespace {

constexpr uint32_t kPlatformInvalidHandleError = 6;

std::filesystem::path ResolveExistingDefaultPetModelPath(const std::string& configuredPathUtf8) {
    const std::string configuredPath = TrimAscii(configuredPathUtf8);
    if (!configuredPath.empty()) {
        const std::filesystem::path preferred(configuredPath);
        if (std::filesystem::exists(preferred)) {
            return preferred;
        }
    }

    static constexpr const char* kCandidates[] = {
        "Assets/Pet3D/source/pet-main.glb",
        "Assets/Pet3D/source/pet-main.vrm",
        "Assets/Pet3D/source/pet-main.usdz",
        "Assets/Pet3D/source/pet-main.gltf",
        "Assets/Pet3D/source/pet-main.fbx",
        "MFCMouseEffect/Assets/Pet3D/source/pet-main.glb",
        "MFCMouseEffect/Assets/Pet3D/source/pet-main.vrm",
        "MFCMouseEffect/Assets/Pet3D/source/pet-main.usdz",
        "MFCMouseEffect/Assets/Pet3D/source/pet-main.gltf",
        "MFCMouseEffect/Assets/Pet3D/source/pet-main.fbx",
    };

    for (const char* candidate : kCandidates) {
        const std::filesystem::path path(candidate);
        if (std::filesystem::exists(path)) {
            return path;
        }
    }
    return {};
}

std::filesystem::path ResolveExistingDefaultPetActionLibraryPath() {
    static constexpr const char* kCandidates[] = {
        "Assets/Pet3D/source/pet-actions.json",
        "MFCMouseEffect/Assets/Pet3D/source/pet-actions.json",
    };

    for (const char* candidate : kCandidates) {
        const std::filesystem::path path(candidate);
        if (std::filesystem::exists(path)) {
            return path;
        }
    }
    return {};
}

std::filesystem::path ResolveExistingDefaultPetActionLibraryPath(const std::string& configuredPathUtf8) {
    const std::string configuredPath = TrimAscii(configuredPathUtf8);
    if (!configuredPath.empty()) {
        const std::filesystem::path preferred(configuredPath);
        if (std::filesystem::exists(preferred)) {
            return preferred;
        }
    }
    return ResolveExistingDefaultPetActionLibraryPath();
}

std::filesystem::path ResolveExistingDefaultPetAppearanceProfilePath() {
    static constexpr const char* kCandidates[] = {
        "Assets/Pet3D/source/pet-appearance.json",
        "MFCMouseEffect/Assets/Pet3D/source/pet-appearance.json",
    };

    for (const char* candidate : kCandidates) {
        const std::filesystem::path path(candidate);
        if (std::filesystem::exists(path)) {
            return path;
        }
    }
    return {};
}

std::filesystem::path ResolveExistingDefaultPetAppearanceProfilePath(const std::string& configuredPathUtf8) {
    const std::string configuredPath = TrimAscii(configuredPathUtf8);
    if (!configuredPath.empty()) {
        const std::filesystem::path preferred(configuredPath);
        if (std::filesystem::exists(preferred)) {
            return preferred;
        }
    }
    return ResolveExistingDefaultPetAppearanceProfilePath();
}

void AppendUniqueExistingPath(
    const std::filesystem::path& candidate,
    std::vector<std::filesystem::path>* outPaths) {
    if (!outPaths || candidate.empty()) {
        return;
    }
    std::error_code ec;
    if (!std::filesystem::exists(candidate, ec) || ec) {
        return;
    }
    for (const auto& existing : *outPaths) {
        if (existing == candidate) {
            return;
        }
    }
    outPaths->push_back(candidate);
}

std::vector<std::filesystem::path> BuildPetVisualModelCandidatePaths(
    const std::filesystem::path& preferredSourcePath,
    const std::string& canonicalGlbPath) {
    std::vector<std::filesystem::path> out;
    out.reserve(3);
    AppendUniqueExistingPath(preferredSourcePath, &out);

    const std::filesystem::path extProbe = preferredSourcePath.empty()
        ? std::filesystem::path(canonicalGlbPath)
        : preferredSourcePath;
    const std::string ext = TrimAscii(extProbe.extension().string());
    if (!ext.empty() && ext != ".usdz" && !extProbe.parent_path().empty() && !extProbe.stem().empty()) {
        AppendUniqueExistingPath(extProbe.parent_path() / (extProbe.stem().string() + ".usdz"), &out);
    }

    AppendUniqueExistingPath(std::filesystem::path(canonicalGlbPath), &out);
    return out;
}

void ResetActionCoverageStatusFields(AppController::MouseCompanionRuntimeStatus* status,
                                     const std::string& error = {}) {
    if (!status) {
        return;
    }
    status->actionCoverageReady = false;
    status->actionCoverageExpectedActionCount = 0;
    status->actionCoverageCoveredActionCount = 0;
    status->actionCoverageMissingActionCount = 0;
    status->actionCoverageSkeletonBoneCount = 0;
    status->actionCoverageTotalTrackCount = 0;
    status->actionCoverageMappedTrackCount = 0;
    status->actionCoverageOverallRatio = 0.0f;
    status->actionCoverageError = error;
    status->actionCoverageMissingActions.clear();
    status->actionCoverageMissingBoneNames.clear();
    status->actionCoverageActions.clear();
}

int ResolveMouseCompanionEdgeClampModeCode(const std::string& mode) {
    const std::string normalized = ToLowerAscii(TrimAscii(mode));
    if (normalized == "strict") {
        return 0;
    }
    if (normalized == "free") {
        return 2;
    }
    return 1; // soft
}

} // namespace

bool AppController::Start() {
    if (dispatchMessageHost_ && dispatchMessageHost_->IsCreated()) return true;
    diag_ = {};
    inputCaptureActive_.store(false, std::memory_order_release);
    inputCaptureError_.store(0, std::memory_order_release);
    effectsSuspendedByInputCapture_.store(false, std::memory_order_release);
    ResetPetDispatchRuntimeState();

    // Load config from the best available directory (AppData preferred)
    configDir_ = ResolveConfigDirectory();
    config_ = EffectConfig::Load(configDir_);
    {
        const MouseCompanionConfig companion = config_internal::SanitizeMouseCompanionConfig(config_.mouseCompanion);
        std::lock_guard<std::mutex> guard(mouseCompanionRuntimeStatusMutex_);
        mouseCompanionRuntimeStatus_ = MouseCompanionRuntimeStatus{};
        mouseCompanionRuntimeStatus_.configEnabled = companion.enabled;
        mouseCompanionRuntimeStatus_.configuredModelPath = companion.modelPath;
        mouseCompanionRuntimeStatus_.configuredActionLibraryPath = companion.actionLibraryPath;
        mouseCompanionRuntimeStatus_.configuredAppearanceProfilePath = companion.appearanceProfilePath;
        mouseCompanionRuntimeStatus_.runtimePresent = (petCompanion_ != nullptr);
    }
    SyncLaunchAtStartupRegistration();
    ReloadThemeCatalogFromRootPath(config_.themeCatalogRootPath);
    const bool themeNormalized = NormalizeConfiguredThemeName();
    QuantumHaloPresenterSelection::SetConfiguredBackendPreference(config_.holdPresenterBackend);
    ApplyOverlayTargetFpsToPlatform();
    InitializeWasmHost();
    inputIndicatorOverlay_->Initialize();
    inputIndicatorOverlay_->UpdateConfig(config_.inputIndicator);
    inputAutomationEngine_.UpdateConfig(config_.automation);
    if (config_.mouseCompanion.enabled) {
        EnsurePetVisualHost();
        TryLoadDefaultPetModel();
    }

    diag_.stage = StartStage::GdiPlusStartup;
    if (!gdiplus_ || !gdiplus_->Startup()) {
#ifdef _DEBUG
        OutputDebugStringW(L"MouseFx: GDI+ startup failed.\n");
#endif
        return false;
    }

    diag_.stage = StartStage::DispatchWindow;
    if (!CreateDispatchWindow()) {
#ifdef _DEBUG
        OutputDebugStringW(L"MouseFx: dispatch window creation failed.\n");
#endif
        Stop();
        return false;
    }

    // Initialize effects with defaults
    diag_.stage = StartStage::EffectInit;
    ApplyConfiguredEffects();
    inputIndicatorOverlay_->UpdateConfig(config_.inputIndicator);

    if (NormalizeActiveEffectTypes() || themeNormalized) {
        PersistConfig();
    }

    lastInputTime_ = CurrentTickMs();
    dispatchMessageHost_->SetTimer(kHoverTimerId, 100);
    dispatchMessageHost_->SetTimer(kInputCaptureHealthTimerId, 500);
    ArmWasmFrameTimer();

    diag_.stage = StartStage::GlobalHook;
    if (!hook_->Start(dispatchMessageHost_.get())) {
        const uint32_t hookError = hook_->LastError();
        inputCaptureActive_.store(false, std::memory_order_release);
        inputCaptureError_.store(hookError, std::memory_order_release);
#ifdef _DEBUG
        wchar_t buf[256]{};
        wsprintfW(buf, L"MouseFx: global hook start failed. GetLastError=%lu\n",
                  static_cast<unsigned long>(hookError));
        OutputDebugStringW(buf);
#endif
#if MFX_PLATFORM_WINDOWS
        diag_.error = static_cast<uint32_t>(hookError);
        Stop();
        return false;
#else
        // macOS/Linux: keep process alive in degraded mode when global capture is unavailable
        // (for example permission not granted yet). UI/tray and local services should continue.
        diag_.error = static_cast<uint32_t>(hookError);
        EnterInputCaptureDegradedMode(hookError);
#endif
    } else {
        inputCaptureActive_.store(true, std::memory_order_release);
        inputCaptureError_.store(0, std::memory_order_release);
        effectsSuspendedByInputCapture_.store(false, std::memory_order_release);
    }

    return true;
}

void AppController::Stop() {
    inputCaptureActive_.store(false, std::memory_order_release);
    inputCaptureError_.store(0, std::memory_order_release);
    effectsSuspendedByInputCapture_.store(false, std::memory_order_release);
    if (dispatchMessageHost_ && dispatchMessageHost_->IsCreated()) {
        dispatchMessageHost_->KillTimer(kHoverTimerId);
        dispatchMessageHost_->KillTimer(kHoldTimerId);
        dispatchMessageHost_->KillTimer(kHoldUpdateTimerId);
        dispatchMessageHost_->KillTimer(kInputCaptureHealthTimerId);
        dispatchMessageHost_->KillTimer(kWasmFrameTimerId);
    }
    ShutdownWasmHost();
    hook_->SetKeyboardCaptureExclusive(false);
    hook_->Stop();
    inputIndicatorOverlay_->Shutdown();
    inputAutomationEngine_.Reset();
    ResetPetDispatchRuntimeState();
    ShutdownPetVisualHost();
    {
        std::lock_guard<std::mutex> guard(mouseCompanionRuntimeStatusMutex_);
        mouseCompanionRuntimeStatus_.runtimePresent = (petCompanion_ != nullptr);
        mouseCompanionRuntimeStatus_.visualHostActive = false;
        mouseCompanionRuntimeStatus_.poseBindingConfigured = false;
    }
    for (auto& effect : effects_) {
        if (effect) {
            effect->Shutdown();
            effect.reset();
        }
    }
    OverlayHostService::Instance().Shutdown();
    DestroyDispatchWindow();
    if (gdiplus_) {
        gdiplus_->Shutdown();
    }
}

bool AppController::CreateDispatchWindow() {
    if (!dispatchMessageHost_) {
        diag_.error = kPlatformInvalidHandleError;
        return false;
    }
    if (dispatchMessageHost_->IsCreated()) {
        return true;
    }
    if (!dispatchMessageHost_->Create(this)) {
        diag_.error = static_cast<uint32_t>(dispatchMessageHost_->LastError());
        return false;
    }
    return true;
}

void AppController::DestroyDispatchWindow() {
    if (dispatchMessageHost_) {
        dispatchMessageHost_->Destroy();
    }
}

void AppController::TryLoadDefaultPetModel() {
    if (!petCompanion_ || !config_.mouseCompanion.enabled) {
        return;
    }

    const MouseCompanionConfig companionCfg =
        config_internal::SanitizeMouseCompanionConfig(config_.mouseCompanion);
    const std::filesystem::path modelPath =
        ResolveExistingDefaultPetModelPath(companionCfg.modelPath);
    {
        std::lock_guard<std::mutex> guard(mouseCompanionRuntimeStatusMutex_);
        mouseCompanionRuntimeStatus_.configuredModelPath = companionCfg.modelPath;
    }
    if (modelPath.empty()) {
        std::lock_guard<std::mutex> guard(mouseCompanionRuntimeStatusMutex_);
        mouseCompanionRuntimeStatus_.modelLoaded = false;
        mouseCompanionRuntimeStatus_.modelLoadError = "model_path_not_found";
        ResetActionCoverageStatusFields(&mouseCompanionRuntimeStatus_, "model_unavailable");
        return;
    }

    auto importer = pet::CreateDefaultModelAssetImporter();
    if (!importer) {
        std::lock_guard<std::mutex> guard(mouseCompanionRuntimeStatusMutex_);
        mouseCompanionRuntimeStatus_.modelLoaded = false;
        mouseCompanionRuntimeStatus_.modelLoadError = "model_importer_unavailable";
        ResetActionCoverageStatusFields(&mouseCompanionRuntimeStatus_, "model_importer_unavailable");
        return;
    }

    pet::CanonicalModelAsset canonical{};
    if (!importer->ImportToCanonicalGlb(modelPath.string(), &canonical)) {
        std::lock_guard<std::mutex> guard(mouseCompanionRuntimeStatusMutex_);
        mouseCompanionRuntimeStatus_.modelLoaded = false;
        mouseCompanionRuntimeStatus_.modelLoadError = "model_import_to_canonical_failed";
        ResetActionCoverageStatusFields(&mouseCompanionRuntimeStatus_, "model_import_failed");
        return;
    }
    if (!petCompanion_->LoadCanonicalModel(canonical)) {
        std::lock_guard<std::mutex> guard(mouseCompanionRuntimeStatusMutex_);
        mouseCompanionRuntimeStatus_.modelLoaded = false;
        mouseCompanionRuntimeStatus_.modelLoadError = "model_runtime_load_failed";
        ResetActionCoverageStatusFields(&mouseCompanionRuntimeStatus_, "model_runtime_load_failed");
        return;
    }
    loadedPetModelPath_ = canonical.canonicalGlbPath;
    {
        std::lock_guard<std::mutex> guard(mouseCompanionRuntimeStatusMutex_);
        mouseCompanionRuntimeStatus_.modelLoaded = true;
        mouseCompanionRuntimeStatus_.modelLoadError.clear();
        mouseCompanionRuntimeStatus_.loadedModelPath = loadedPetModelPath_;
        const pet::SkeletonDesc* skeleton = petCompanion_ ? petCompanion_->CurrentSkeleton() : nullptr;
        mouseCompanionRuntimeStatus_.skeletonBoneCount =
            skeleton ? static_cast<int>(skeleton->bones.size()) : 0;
    }
    const std::vector<std::filesystem::path> visualCandidates =
        BuildPetVisualModelCandidatePaths(modelPath, loadedPetModelPath_);
    bool visualLoaded = false;
    for (const auto& visualCandidate : visualCandidates) {
        if (TryLoadPetModelIntoVisualHost(visualCandidate.string())) {
            visualLoaded = true;
            break;
        }
    }
    if (!visualLoaded && petVisualHostHandle_ != nullptr) {
        std::lock_guard<std::mutex> guard(mouseCompanionRuntimeStatusMutex_);
        mouseCompanionRuntimeStatus_.visualModelLoaded = false;
        if (mouseCompanionRuntimeStatus_.visualModelLoadError.empty()) {
            mouseCompanionRuntimeStatus_.visualModelLoadError = "visual_model_candidate_not_loadable";
        }
    }
    TryLoadDefaultPetActionLibrary();
    TryLoadDefaultPetAppearanceProfile();
}

void AppController::TryLoadDefaultPetActionLibrary() {
    if (!petCompanion_) {
        return;
    }
    const MouseCompanionConfig companionCfg =
        config_internal::SanitizeMouseCompanionConfig(config_.mouseCompanion);
    const std::filesystem::path libraryPath =
        ResolveExistingDefaultPetActionLibraryPath(companionCfg.actionLibraryPath);
    {
        std::lock_guard<std::mutex> guard(mouseCompanionRuntimeStatusMutex_);
        mouseCompanionRuntimeStatus_.configuredActionLibraryPath = companionCfg.actionLibraryPath;
    }
    if (libraryPath.empty()) {
        loadedPetActionLibraryPath_.clear();
        std::lock_guard<std::mutex> guard(mouseCompanionRuntimeStatusMutex_);
        mouseCompanionRuntimeStatus_.actionLibraryLoaded = false;
        mouseCompanionRuntimeStatus_.actionLibraryLoadError = "action_library_path_not_found";
        ResetActionCoverageStatusFields(&mouseCompanionRuntimeStatus_, "action_library_unavailable");
        mouseCompanionRuntimeStatus_.loadedActionLibraryPath.clear();
        return;
    }
    if (!petCompanion_->LoadActionLibraryFromJson(libraryPath.string())) {
        std::lock_guard<std::mutex> guard(mouseCompanionRuntimeStatusMutex_);
        mouseCompanionRuntimeStatus_.actionLibraryLoaded = false;
        mouseCompanionRuntimeStatus_.actionLibraryLoadError = "action_library_parse_or_bind_failed";
        ResetActionCoverageStatusFields(&mouseCompanionRuntimeStatus_, "action_library_parse_or_bind_failed");
        return;
    }
    loadedPetActionLibraryPath_ = libraryPath.string();
    {
        std::lock_guard<std::mutex> guard(mouseCompanionRuntimeStatusMutex_);
        mouseCompanionRuntimeStatus_.actionLibraryLoaded = true;
        mouseCompanionRuntimeStatus_.actionLibraryLoadError.clear();
        mouseCompanionRuntimeStatus_.loadedActionLibraryPath = loadedPetActionLibraryPath_;
    }
    RecomputePetActionCoverageStatus();
}

void AppController::TryLoadDefaultPetAppearanceProfile() {
    if (!petCompanion_) {
        return;
    }
    const MouseCompanionConfig companionCfg =
        config_internal::SanitizeMouseCompanionConfig(config_.mouseCompanion);
    const std::filesystem::path profilePath =
        ResolveExistingDefaultPetAppearanceProfilePath(companionCfg.appearanceProfilePath);
    {
        std::lock_guard<std::mutex> guard(mouseCompanionRuntimeStatusMutex_);
        mouseCompanionRuntimeStatus_.configuredAppearanceProfilePath = companionCfg.appearanceProfilePath;
    }
    if (profilePath.empty()) {
        loadedPetAppearanceProfilePath_.clear();
        std::lock_guard<std::mutex> guard(mouseCompanionRuntimeStatusMutex_);
        mouseCompanionRuntimeStatus_.appearanceProfileLoaded = false;
        mouseCompanionRuntimeStatus_.appearanceProfileLoadError = "appearance_profile_path_not_found";
        return;
    }

    pet::PetAppearanceProfile profile{};
    std::string error;
    if (!pet::LoadPetAppearanceProfileFromJsonFile(profilePath.string(), &profile, &error)) {
        (void)error;
        std::lock_guard<std::mutex> guard(mouseCompanionRuntimeStatusMutex_);
        mouseCompanionRuntimeStatus_.appearanceProfileLoaded = false;
        mouseCompanionRuntimeStatus_.appearanceProfileLoadError = "appearance_profile_parse_failed";
        return;
    }
    petCompanion_->ApplyAppearance(profile.defaultAppearance);
    loadedPetAppearanceProfilePath_ = profilePath.string();
    {
        std::lock_guard<std::mutex> guard(mouseCompanionRuntimeStatusMutex_);
        mouseCompanionRuntimeStatus_.appearanceProfileLoaded = true;
        mouseCompanionRuntimeStatus_.appearanceProfileLoadError.clear();
        mouseCompanionRuntimeStatus_.loadedAppearanceProfilePath = loadedPetAppearanceProfilePath_;
    }
    TryApplyPetAppearanceToVisualHost();
}

void AppController::RecomputePetActionCoverageStatus() {
    {
        std::lock_guard<std::mutex> guard(mouseCompanionRuntimeStatusMutex_);
        ResetActionCoverageStatusFields(&mouseCompanionRuntimeStatus_);
    }

    if (!petCompanion_) {
        std::lock_guard<std::mutex> guard(mouseCompanionRuntimeStatusMutex_);
        ResetActionCoverageStatusFields(&mouseCompanionRuntimeStatus_, "runtime_unavailable");
        return;
    }
    const pet::SkeletonDesc* skeleton = petCompanion_->CurrentSkeleton();
    if (!skeleton || skeleton->bones.empty()) {
        std::lock_guard<std::mutex> guard(mouseCompanionRuntimeStatusMutex_);
        ResetActionCoverageStatusFields(&mouseCompanionRuntimeStatus_, "skeleton_unavailable");
        return;
    }
    if (loadedPetActionLibraryPath_.empty()) {
        std::lock_guard<std::mutex> guard(mouseCompanionRuntimeStatusMutex_);
        ResetActionCoverageStatusFields(&mouseCompanionRuntimeStatus_, "action_library_unavailable");
        return;
    }

    pet::ActionLibrary library{};
    std::string error;
    if (!pet::LoadActionLibraryFromJsonFile(loadedPetActionLibraryPath_, &library, &error)) {
        std::lock_guard<std::mutex> guard(mouseCompanionRuntimeStatusMutex_);
        ResetActionCoverageStatusFields(
            &mouseCompanionRuntimeStatus_,
            error.empty() ? std::string("action_library_reparse_failed") : error);
        return;
    }

    const pet::ActionCoverageReport report = pet::BuildActionCoverageReport(*skeleton, library);
    std::lock_guard<std::mutex> guard(mouseCompanionRuntimeStatusMutex_);
    mouseCompanionRuntimeStatus_.actionCoverageReady = true;
    mouseCompanionRuntimeStatus_.actionCoverageExpectedActionCount = report.expectedActionCount;
    mouseCompanionRuntimeStatus_.actionCoverageCoveredActionCount = report.coveredActionCount;
    mouseCompanionRuntimeStatus_.actionCoverageMissingActionCount = report.missingActionCount;
    mouseCompanionRuntimeStatus_.actionCoverageSkeletonBoneCount = report.skeletonBoneCount;
    mouseCompanionRuntimeStatus_.actionCoverageTotalTrackCount = report.totalTrackCount;
    mouseCompanionRuntimeStatus_.actionCoverageMappedTrackCount = report.totalMappedTrackCount;
    mouseCompanionRuntimeStatus_.actionCoverageOverallRatio = report.overallCoverageRatio;
    mouseCompanionRuntimeStatus_.actionCoverageError.clear();
    mouseCompanionRuntimeStatus_.actionCoverageMissingActions = report.missingActions;
    mouseCompanionRuntimeStatus_.actionCoverageMissingBoneNames = report.missingBoneNames;
    mouseCompanionRuntimeStatus_.actionCoverageActions.clear();
    mouseCompanionRuntimeStatus_.actionCoverageActions.reserve(report.actions.size());
    for (const auto& action : report.actions) {
        MouseCompanionRuntimeStatus::ActionCoverageActionStatus mapped{};
        mapped.actionName = action.actionName;
        mapped.clipPresent = action.clipPresent;
        mapped.trackCount = action.trackCount;
        mapped.mappedTrackCount = action.mappedTrackCount;
        mapped.coverageRatio = action.coverageRatio;
        mapped.missingBoneTracks = action.missingBoneTracks;
        mouseCompanionRuntimeStatus_.actionCoverageActions.push_back(std::move(mapped));
    }
}

void AppController::EnsurePetVisualHost() {
#if MFX_PLATFORM_MACOS
    if (!config_.mouseCompanion.enabled) {
        return;
    }
    if (petVisualHostHandle_ == nullptr) {
        petVisualHostHandle_ = mfx_macos_mouse_companion_create_v1(config_.mouseCompanion.sizePx);
    }
    if (petVisualHostHandle_ != nullptr) {
        ApplyPetVisualFollowProfile();
        mfx_macos_mouse_companion_show_v1(petVisualHostHandle_);
        {
            std::lock_guard<std::mutex> guard(mouseCompanionRuntimeStatusMutex_);
            mouseCompanionRuntimeStatus_.visualHostActive = true;
        }
        const std::filesystem::path preferredSourcePath =
            ResolveExistingDefaultPetModelPath(config_.mouseCompanion.modelPath);
        const std::vector<std::filesystem::path> visualCandidates =
            BuildPetVisualModelCandidatePaths(preferredSourcePath, loadedPetModelPath_);
        for (const auto& visualCandidate : visualCandidates) {
            if (TryLoadPetModelIntoVisualHost(visualCandidate.string())) {
                break;
            }
        }
        TryApplyPetAppearanceToVisualHost();
    }
#endif
}

void AppController::ApplyPetVisualFollowProfile() {
#if MFX_PLATFORM_MACOS
    if (petVisualHostHandle_ == nullptr || !config_.mouseCompanion.enabled) {
        return;
    }
    const MouseCompanionConfig& cfg = config_.mouseCompanion;
    const int pressLiftPx = cfg.useTestProfile ? cfg.testPressLiftPx : cfg.pressLiftPx;
    const int edgeClampModeCode = ResolveMouseCompanionEdgeClampModeCode(cfg.edgeClampMode);
    mfx_macos_mouse_companion_configure_follow_profile_v1(
        petVisualHostHandle_,
        cfg.offsetX,
        cfg.offsetY,
        pressLiftPx,
        edgeClampModeCode);
#endif
}

void AppController::ShutdownPetVisualHost() {
#if MFX_PLATFORM_MACOS
    if (petVisualHostHandle_ == nullptr) {
        return;
    }
    mfx_macos_mouse_companion_hide_v1(petVisualHostHandle_);
    mfx_macos_mouse_companion_release_v1(petVisualHostHandle_);
    petVisualHostHandle_ = nullptr;
    petVisualPoseBindingConfigured_ = false;
    petVisualSkeletonNamePtrs_.clear();
    petVisualSkeletonNames_.clear();
    {
        std::lock_guard<std::mutex> guard(mouseCompanionRuntimeStatusMutex_);
        mouseCompanionRuntimeStatus_.visualHostActive = false;
        mouseCompanionRuntimeStatus_.visualModelLoaded = false;
        mouseCompanionRuntimeStatus_.poseBindingConfigured = false;
        mouseCompanionRuntimeStatus_.visualModelPath.clear();
        mouseCompanionRuntimeStatus_.visualModelLoadError.clear();
    }
#endif
}

bool AppController::TryLoadPetModelIntoVisualHost(const std::string& modelPath) {
#if MFX_PLATFORM_MACOS
    if (petVisualHostHandle_ == nullptr || modelPath.empty()) {
        return false;
    }
    // Model reload invalidates previous binding cache; force rebinding against the new scene graph.
    petVisualPoseBindingConfigured_ = false;
    petVisualSkeletonNamePtrs_.clear();
    petVisualSkeletonNames_.clear();
    {
        std::lock_guard<std::mutex> guard(mouseCompanionRuntimeStatusMutex_);
        mouseCompanionRuntimeStatus_.poseBindingConfigured = false;
        mouseCompanionRuntimeStatus_.visualModelPath = modelPath;
        mouseCompanionRuntimeStatus_.visualModelLoaded = false;
    }
    if (mfx_macos_mouse_companion_load_model_v1(petVisualHostHandle_, modelPath.c_str()) != 0) {
        {
            std::lock_guard<std::mutex> guard(mouseCompanionRuntimeStatusMutex_);
            mouseCompanionRuntimeStatus_.visualModelLoaded = true;
            mouseCompanionRuntimeStatus_.visualModelLoadError.clear();
        }
        // Pre-bind pose mapping immediately after model load so runtime readiness is observable
        // even before the first pointer-driven animation tick.
        (void)EnsurePetVisualPoseBinding();
        TryApplyPetAppearanceToVisualHost();
        return true;
    }
    {
        std::lock_guard<std::mutex> guard(mouseCompanionRuntimeStatusMutex_);
        mouseCompanionRuntimeStatus_.visualModelLoaded = false;
        mouseCompanionRuntimeStatus_.visualModelLoadError = "visual_model_load_failed";
    }
    return false;
#else
    (void)modelPath;
    return false;
#endif
}

} // namespace mousefx
