#include "pch.h"

#include "AppController.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include "MouseFx/Core/Overlay/InputIndicatorKeyFilter.h"
#include "MouseFx/Core/Pet/PetCompanionRuntime.h"
#include "Platform/PlatformTarget.h"
#if MFX_PLATFORM_MACOS
#include "Platform/macos/Effects/MacosOverlayRenderSupport.h"
#include "Platform/macos/Overlay/MacosOverlayCoordSpaceConversion.h"
#include "Platform/macos/Pet/MacosMouseCompanionSwiftBridge.h"
#elif MFX_PLATFORM_WINDOWS
#include "Platform/windows/Overlay/Win32OverlayTimerSupport.h"
#endif

namespace mousefx {
namespace {

std::string ResolveInputIndicatorWasmRouteReason(
    const InputIndicatorWasmRouteTrace& trace,
    bool renderedByWasm,
    bool wasmFallbackEnabled,
    bool nativeFallbackApplied) {
    if (renderedByWasm) {
        return "wasm_rendered";
    }
    if (!wasmFallbackEnabled) {
        return "fallback_disabled";
    }
    if (!trace.anchorsResolved) {
        return "anchor_unavailable";
    }
    if (!trace.hostPresent || !trace.hostEnabled || !trace.pluginLoaded) {
        return "plugin_unloaded";
    }
    if (!trace.eventSupported) {
        return "event_not_supported";
    }
    if (trace.invokeAttempted) {
        return nativeFallbackApplied ? "invoke_failed_no_output" : "invoke_no_output";
    }
    return nativeFallbackApplied ? "invoke_failed_no_output" : "unknown";
}

double ResolvePetDeltaSeconds(uint64_t* inOutLastTickMs, uint64_t nowTickMs) {
    if (!inOutLastTickMs) {
        return 1.0 / 60.0;
    }
    if (*inOutLastTickMs == 0 || nowTickMs <= *inOutLastTickMs) {
        *inOutLastTickMs = nowTickMs;
        return 1.0 / 60.0;
    }
    const uint64_t deltaMs = nowTickMs - *inOutLastTickMs;
    *inOutLastTickMs = nowTickMs;
    const double dt = static_cast<double>(deltaMs) / 1000.0;
    return std::clamp(dt, 1.0 / 240.0, 1.0 / 15.0);
}

pet::PetFrameInput BuildPetFrameInput(const ScreenPoint& pt,
                                      bool primaryPressed,
                                      bool dragging,
                                      double dtSeconds) {
    pet::PetFrameInput input{};
    input.dtSeconds = dtSeconds;
    input.cursorVisible = true;
    input.cursorPosition.x = static_cast<float>(pt.x);
    input.cursorPosition.y = static_cast<float>(pt.y);
    input.cursorPosition.z = 0.0f;
    input.primaryPressed = primaryPressed;
    input.dragging = dragging;
    return input;
}

MouseCompanionConfig ResolveActiveMouseCompanionConfig(const MouseCompanionConfig& config) {
    MouseCompanionConfig active = config;
    if (active.useTestProfile) {
        active.pressLiftPx = active.testPressLiftPx;
        active.smoothingPercent = active.testSmoothingPercent;
    }
    return active;
}

double ResolvePetSmoothingAlpha(int smoothingPercent, double dtSeconds) {
    const int clampedSmoothing = std::clamp(smoothingPercent, 0, 95);
    if (clampedSmoothing <= 0) {
        return 1.0;
    }
    const double dt = std::max(0.0, dtSeconds);
    const double frameScale = std::max(1.0, dt * 60.0);
    const double keepRatio = static_cast<double>(clampedSmoothing) / 100.0;
    const double preserved = std::pow(keepRatio, frameScale);
    return std::clamp(1.0 - preserved, 0.01, 1.0);
}

int64_t DistanceSquared(const ScreenPoint& lhs, const ScreenPoint& rhs) {
    const int64_t dx = static_cast<int64_t>(lhs.x) - static_cast<int64_t>(rhs.x);
    const int64_t dy = static_cast<int64_t>(lhs.y) - static_cast<int64_t>(rhs.y);
    return (dx * dx) + (dy * dy);
}

const char* ResolveMouseCompanionActionName(int actionCode) {
    switch (static_cast<pet::PetAction>(actionCode)) {
    case pet::PetAction::Idle:
        return "idle";
    case pet::PetAction::Follow:
        return "follow";
    case pet::PetAction::ClickReact:
        return "click_react";
    case pet::PetAction::Drag:
        return "drag";
    case pet::PetAction::HoverReact:
        return "hover_react";
    case pet::PetAction::HoldReact:
        return "hold_react";
    case pet::PetAction::ScrollReact:
        return "scroll_react";
    default:
        return "unknown";
    }
}

#if MFX_PLATFORM_MACOS
struct PetVisualPosePayload final {
    std::vector<int> boneIndices{};
    std::vector<float> positions{};
    std::vector<float> rotations{};
    std::vector<float> scales{};

    int Count() const {
        return static_cast<int>(boneIndices.size());
    }
};

int ResolvePoseBoneIndex(const pet::BonePose& bonePose, const pet::SkeletonDesc* skeleton) {
    if (bonePose.boneIndex >= 0) {
        const int direct = bonePose.boneIndex;
        if (!skeleton) {
            return direct;
        }
        const size_t index = static_cast<size_t>(direct);
        if (index < skeleton->bones.size()) {
            return direct;
        }
    }
    if (!skeleton || bonePose.name.empty()) {
        return -1;
    }
    for (size_t i = 0; i < skeleton->bones.size(); ++i) {
        if (skeleton->bones[i].name == bonePose.name) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

PetVisualPosePayload BuildPetVisualPosePayload(const pet::PetCompanionRuntime* companion) {
    PetVisualPosePayload payload{};
    if (!companion) {
        return payload;
    }

    const pet::SkeletonPose& pose = companion->LastPose();
    if (pose.bones.empty()) {
        return payload;
    }

    const pet::SkeletonDesc* skeleton = companion->CurrentSkeleton();
    constexpr size_t kMaxPosesPerFrame = 64;
    const size_t reserveCount = std::min(kMaxPosesPerFrame, pose.bones.size());
    payload.boneIndices.reserve(reserveCount);
    payload.positions.reserve(reserveCount * 3);
    payload.rotations.reserve(reserveCount * 4);
    payload.scales.reserve(reserveCount * 3);

    for (const auto& bonePose : pose.bones) {
        if (payload.boneIndices.size() >= kMaxPosesPerFrame) {
            break;
        }

        const int boneIndex = ResolvePoseBoneIndex(bonePose, skeleton);
        if (boneIndex < 0) {
            continue;
        }

        payload.boneIndices.push_back(boneIndex);
        payload.positions.push_back(bonePose.local.position.x);
        payload.positions.push_back(bonePose.local.position.y);
        payload.positions.push_back(bonePose.local.position.z);
        payload.rotations.push_back(bonePose.local.rotation.x);
        payload.rotations.push_back(bonePose.local.rotation.y);
        payload.rotations.push_back(bonePose.local.rotation.z);
        payload.rotations.push_back(bonePose.local.rotation.w);
        payload.scales.push_back(bonePose.local.scale.x);
        payload.scales.push_back(bonePose.local.scale.y);
        payload.scales.push_back(bonePose.local.scale.z);
    }

    return payload;
}
#endif

} // namespace

bool AppController::ConsumeIgnoreNextClick() {
    if (!ignoreNextClick_) {
        return false;
    }
    ignoreNextClick_ = false;
    return true;
}

void AppController::OnGlobalKey(const KeyEvent& ev) {
    InputAutomation().OnKey(ev);
    if (!ev.keyDown) {
        return;
    }
    const bool captureActiveBefore = shortcutCaptureSession_.IsActive();
    shortcutCaptureSession_.OnKeyDown(ev);
    const bool captureActiveAfter = shortcutCaptureSession_.IsActive();
    hook_->SetKeyboardCaptureExclusive(captureActiveAfter);
    if (captureActiveBefore || captureActiveAfter) {
        return;
    }
    DispatchInputIndicatorKey(ev);
}

void AppController::DispatchInputIndicatorClick(const ClickEvent& ev) {
    const InputIndicatorConfig& cfg = config_.inputIndicator;
    if (!cfg.enabled) {
        return;
    }

    if (cfg.renderMode == "wasm") {
        InputIndicatorWasmRouteTrace trace{};
        bool renderedByWasm = false;
        inputIndicatorWasmDispatch_.RouteClick(*this, ev, &renderedByWasm, &trace);
        const bool nativeFallbackApplied = !renderedByWasm && cfg.wasmFallbackToNative;
        RecordInputIndicatorWasmRouteStatus(
            "click", trace, renderedByWasm, cfg.wasmFallbackToNative, nativeFallbackApplied);
        if (renderedByWasm || !cfg.wasmFallbackToNative) {
            return;
        }
    }
    inputIndicatorOverlay_->OnClick(ev);
}

void AppController::DispatchInputIndicatorScroll(const ScrollEvent& ev) {
    const InputIndicatorConfig& cfg = config_.inputIndicator;
    if (!cfg.enabled) {
        return;
    }

    if (cfg.renderMode == "wasm") {
        InputIndicatorWasmRouteTrace trace{};
        bool renderedByWasm = false;
        inputIndicatorWasmDispatch_.RouteScroll(*this, ev, &renderedByWasm, &trace);
        const bool nativeFallbackApplied = !renderedByWasm && cfg.wasmFallbackToNative;
        RecordInputIndicatorWasmRouteStatus(
            "scroll", trace, renderedByWasm, cfg.wasmFallbackToNative, nativeFallbackApplied);
        if (renderedByWasm || !cfg.wasmFallbackToNative) {
            return;
        }
    }
    inputIndicatorOverlay_->OnScroll(ev);
}

void AppController::DispatchInputIndicatorKey(const KeyEvent& ev) {
    const InputIndicatorConfig& cfg = config_.inputIndicator;
    if (!ShouldShowInputIndicatorKey(cfg, ev)) {
        return;
    }

    if (cfg.renderMode == "wasm") {
        InputIndicatorWasmRouteTrace trace{};
        bool renderedByWasm = false;
        inputIndicatorWasmDispatch_.RouteKey(*this, ev, &renderedByWasm, &trace);
        const bool nativeFallbackApplied = !renderedByWasm && cfg.wasmFallbackToNative;
        RecordInputIndicatorWasmRouteStatus(
            "key", trace, renderedByWasm, cfg.wasmFallbackToNative, nativeFallbackApplied);
        if (renderedByWasm || !cfg.wasmFallbackToNative) {
            return;
        }
    }
    inputIndicatorOverlay_->OnKey(ev);
}

AppController::InputIndicatorWasmRouteStatus AppController::ReadInputIndicatorWasmRouteStatus() const {
    std::lock_guard<std::mutex> guard(inputIndicatorWasmRouteStatusMutex_);
    return inputIndicatorWasmRouteStatus_;
}

AppController::MouseCompanionRuntimeStatus AppController::ReadMouseCompanionRuntimeStatus() const {
    std::lock_guard<std::mutex> guard(mouseCompanionRuntimeStatusMutex_);
    return mouseCompanionRuntimeStatus_;
}

void AppController::RecordInputIndicatorWasmRouteStatus(
    const char* eventKind,
    const InputIndicatorWasmRouteTrace& trace,
    bool renderedByWasm,
    bool wasmFallbackEnabled,
    bool nativeFallbackApplied) {
    InputIndicatorWasmRouteStatus next{};
    next.eventKind = eventKind ? eventKind : "";
    next.renderMode = config_.inputIndicator.renderMode;
    next.eventTickMs = CurrentTickMs();
    next.routeAttempted = trace.routeAttempted;
    next.anchorsResolved = trace.anchorsResolved;
    next.hostPresent = trace.hostPresent;
    next.hostEnabled = trace.hostEnabled;
    next.pluginLoaded = trace.pluginLoaded;
    next.eventSupported = trace.eventSupported;
    next.invokeAttempted = trace.invokeAttempted;
    next.renderedByWasm = renderedByWasm || trace.renderedAny;
    next.wasmFallbackEnabled = wasmFallbackEnabled;
    next.nativeFallbackApplied = nativeFallbackApplied;
    next.reason = ResolveInputIndicatorWasmRouteReason(
        trace,
        next.renderedByWasm,
        wasmFallbackEnabled,
        nativeFallbackApplied);

    std::lock_guard<std::mutex> guard(inputIndicatorWasmRouteStatusMutex_);
    inputIndicatorWasmRouteStatus_ = std::move(next);
}

std::string AppController::StartShortcutCaptureSession(uint64_t timeoutMs) {
    const std::string sessionId = shortcutCaptureSession_.Start(timeoutMs);
    hook_->SetKeyboardCaptureExclusive(shortcutCaptureSession_.IsActive());
    return sessionId;
}

void AppController::StopShortcutCaptureSession(const std::string& sessionId) {
    shortcutCaptureSession_.Stop(sessionId);
    hook_->SetKeyboardCaptureExclusive(shortcutCaptureSession_.IsActive());
}

ShortcutCaptureSession::PollResult AppController::PollShortcutCaptureSession(const std::string& sessionId) {
    ShortcutCaptureSession::PollResult result = shortcutCaptureSession_.Poll(sessionId);
    hook_->SetKeyboardCaptureExclusive(shortcutCaptureSession_.IsActive());
    return result;
}

bool AppController::ConsumeLatestMove(ScreenPoint* outPt) {
    if (!outPt) {
        return false;
    }
    return hook_->ConsumeLatestMove(*outPt);
}

uint64_t AppController::CurrentTickMs() const {
    if (!monotonicClockService_) {
        return 0;
    }
    return monotonicClockService_->NowMs();
}

uint32_t AppController::CurrentHoldDurationMs() const {
    if (!holdButtonDown_ || holdDownTick_ == 0) {
        return 0;
    }

    const uint64_t now = CurrentTickMs();
    const uint64_t delta = (now >= holdDownTick_) ? (now - holdDownTick_) : 0;
    return static_cast<uint32_t>(std::min<uint64_t>(delta, 0xFFFFFFFFu));
}

uint32_t AppController::ActiveHoverThresholdMs() const {
    const MouseCompanionConfig activeConfig = ResolveActiveMouseCompanionConfig(config_.mouseCompanion);
    return activeConfig.useTestProfile ? kHoverThresholdTestMs : kHoverThresholdMs;
}

uint32_t AppController::ActiveHoldDelayMs() const {
    const MouseCompanionConfig activeConfig = ResolveActiveMouseCompanionConfig(config_.mouseCompanion);
    return activeConfig.useTestProfile ? kHoldDelayTestMs : kHoldDelayMs;
}

void AppController::BeginHoldTracking(const ScreenPoint& pt, int button) {
    holdButtonDown_ = true;
    holdTrackingButton_ = button;
    holdDownTick_ = CurrentTickMs();
    pendingHold_.pt = pt;
    pendingHold_.button = button;
    pendingHold_.active = true;
    ignoreNextClick_ = false;
}

void AppController::EndHoldTracking() {
    holdButtonDown_ = false;
    holdTrackingButton_ = 0;
    holdDownTick_ = 0;
}

void AppController::ArmHoldTimer() {
    if (dispatchMessageHost_ && dispatchMessageHost_->IsCreated()) {
        dispatchMessageHost_->SetTimer(kHoldTimerId, ActiveHoldDelayMs());
    }
}

void AppController::DisarmHoldTimer() {
    if (dispatchMessageHost_ && dispatchMessageHost_->IsCreated()) {
        dispatchMessageHost_->KillTimer(kHoldTimerId);
    }
}

void AppController::ArmHoldUpdateTimer() {
    if (dispatchMessageHost_ && dispatchMessageHost_->IsCreated()) {
        dispatchMessageHost_->SetTimer(kHoldUpdateTimerId, kHoldUpdateIntervalMs);
    }
}

void AppController::DisarmHoldUpdateTimer() {
    if (dispatchMessageHost_ && dispatchMessageHost_->IsCreated()) {
        dispatchMessageHost_->KillTimer(kHoldUpdateTimerId);
    }
}

void AppController::ArmWasmFrameTimer() {
    if (!dispatchMessageHost_ || !dispatchMessageHost_->IsCreated()) {
        return;
    }
    dispatchMessageHost_->SetTimer(kWasmFrameTimerId, ResolveWasmFrameTimerIntervalMs());
}

void AppController::DisarmWasmFrameTimer() {
    if (!dispatchMessageHost_ || !dispatchMessageHost_->IsCreated()) {
        return;
    }
    dispatchMessageHost_->KillTimer(kWasmFrameTimerId);
}

void AppController::ClearPendingHold() {
    pendingHold_.active = false;
}

void AppController::CancelPendingHold() {
    if (!pendingHold_.active) {
        return;
    }
    if (dispatchMessageHost_ && dispatchMessageHost_->IsCreated()) {
        dispatchMessageHost_->KillTimer(kHoldTimerId);
    }
    pendingHold_.active = false;
}

bool AppController::ConsumePendingHold(ScreenPoint* outPt, int* outButton) {
    if (!pendingHold_.active) {
        return false;
    }
    if (outPt) {
        *outPt = pendingHold_.pt;
    }
    if (outButton) {
        *outButton = pendingHold_.button;
    }
    pendingHold_.active = false;
    return true;
}

void AppController::MarkIgnoreNextClick() {
    ignoreNextClick_ = true;
}

bool AppController::TryEnterHover(ScreenPoint* outPt) {
    if (hovering_) {
        return false;
    }

    const uint64_t elapsed = CurrentTickMs() - lastInputTime_;
    if (elapsed < ActiveHoverThresholdMs()) {
        return false;
    }

    hovering_ = true;
    if (outPt) {
        if (QueryCursorScreenPoint(outPt)) {
            RememberLastPointerPoint(*outPt);
        } else if (!TryGetLastPointerPoint(outPt)) {
            outPt->x = 0;
            outPt->y = 0;
        }
    }
    return true;
}

bool AppController::QueryCursorScreenPoint(ScreenPoint* outPt) const {
    if (!outPt || !cursorPositionService_) {
        return false;
    }
    return cursorPositionService_->TryGetCursorScreenPoint(outPt);
}

void AppController::DispatchPetMove(const ScreenPoint& pt) {
    if (!config_.mouseCompanion.enabled || !petCompanion_) {
        return;
    }
    const MouseCompanionConfig activeConfig = ResolveActiveMouseCompanionConfig(config_.mouseCompanion);
    const uint64_t nowTickMs = CurrentTickMs();
    const bool releaseHoldActive =
        (activeConfig.releaseHoldMs > 0) &&
        (petReleaseHoldUntilTickMs_ > nowTickMs);
    const bool effectiveDragging = petDragging_ || releaseHoldActive;
    if (!effectiveDragging &&
        activeConfig.followThresholdPx > 0 &&
        petHasLastDispatchPoint_) {
        const int64_t threshold = static_cast<int64_t>(activeConfig.followThresholdPx);
        if (DistanceSquared(pt, petLastDispatchPoint_) < (threshold * threshold)) {
            return;
        }
    }
    petLastDispatchPoint_ = pt;
    petHasLastDispatchPoint_ = true;

    const double dt = ResolvePetDeltaSeconds(&petLastTickMs_, nowTickMs);
    const ScreenPoint runtimePt = ResolvePetRuntimeCursorPoint(pt, dt, activeConfig.smoothingPercent);
    if (effectiveDragging) {
        petCompanion_->RequestAction(pet::PetAction::Drag, 0.05);
    } else {
        petCompanion_->RequestAction(pet::PetAction::Follow, 0.12);
    }
    petCompanion_->Tick(BuildPetFrameInput(runtimePt, holdButtonDown_, effectiveDragging, dt));
    UpdatePetVisualState(
        runtimePt,
        effectiveDragging ? static_cast<int>(pet::PetAction::Drag) : static_cast<int>(pet::PetAction::Follow),
        effectiveDragging ? 0.8f : 0.6f);
}

void AppController::DispatchPetScroll(const ScreenPoint& pt, int delta) {
    if (!config_.mouseCompanion.enabled || !petCompanion_) {
        return;
    }
    const MouseCompanionConfig activeConfig = ResolveActiveMouseCompanionConfig(config_.mouseCompanion);
    petLastDispatchPoint_ = pt;
    petHasLastDispatchPoint_ = true;
    const uint64_t nowTickMs = CurrentTickMs();
    const double dt = ResolvePetDeltaSeconds(&petLastTickMs_, nowTickMs);
    const ScreenPoint runtimePt = ResolvePetRuntimeCursorPoint(pt, dt, activeConfig.smoothingPercent);
    petCompanion_->RequestAction(pet::PetAction::ScrollReact, 0.05);
    petCompanion_->Tick(BuildPetFrameInput(runtimePt, holdButtonDown_, petDragging_, dt));
    const float intensity = std::clamp(static_cast<float>(std::abs(delta)) / 120.0f, 0.45f, 1.0f);
    UpdatePetVisualState(runtimePt, static_cast<int>(pet::PetAction::ScrollReact), intensity);
}

void AppController::DispatchPetClick(const ClickEvent& ev) {
    if (!config_.mouseCompanion.enabled || !petCompanion_) {
        return;
    }
    const MouseCompanionConfig activeConfig = ResolveActiveMouseCompanionConfig(config_.mouseCompanion);
    petReleaseHoldUntilTickMs_ = 0;
    petLastDispatchPoint_ = ev.pt;
    petHasLastDispatchPoint_ = true;
    const bool primary = (ev.button == MouseButton::Left);
    const uint64_t nowTickMs = CurrentTickMs();
    const double dt = ResolvePetDeltaSeconds(&petLastTickMs_, nowTickMs);
    const ScreenPoint runtimePt = ResolvePetRuntimeCursorPoint(ev.pt, dt, activeConfig.smoothingPercent);
    petCompanion_->RequestAction(pet::PetAction::ClickReact, 0.04);
    petCompanion_->Tick(BuildPetFrameInput(runtimePt, primary, petDragging_, dt));
    UpdatePetVisualState(runtimePt, static_cast<int>(pet::PetAction::ClickReact), 0.95f);
}

void AppController::DispatchPetHoverStart(const ScreenPoint& pt) {
    if (!config_.mouseCompanion.enabled || !petCompanion_) {
        return;
    }
    const MouseCompanionConfig activeConfig = ResolveActiveMouseCompanionConfig(config_.mouseCompanion);
    petLastDispatchPoint_ = pt;
    petHasLastDispatchPoint_ = true;
    const uint64_t nowTickMs = CurrentTickMs();
    const double dt = ResolvePetDeltaSeconds(&petLastTickMs_, nowTickMs);
    const ScreenPoint runtimePt = ResolvePetRuntimeCursorPoint(pt, dt, activeConfig.smoothingPercent);
    petCompanion_->RequestAction(pet::PetAction::HoverReact, 0.10);
    petCompanion_->Tick(BuildPetFrameInput(runtimePt, false, false, dt));
    UpdatePetVisualState(runtimePt, static_cast<int>(pet::PetAction::HoverReact), 0.58f);
}

void AppController::DispatchPetHoverEnd(const ScreenPoint& pt) {
    if (!config_.mouseCompanion.enabled || !petCompanion_) {
        return;
    }
    const MouseCompanionConfig activeConfig = ResolveActiveMouseCompanionConfig(config_.mouseCompanion);
    petLastDispatchPoint_ = pt;
    petHasLastDispatchPoint_ = true;
    const uint64_t nowTickMs = CurrentTickMs();
    const double dt = ResolvePetDeltaSeconds(&petLastTickMs_, nowTickMs);
    const ScreenPoint runtimePt = ResolvePetRuntimeCursorPoint(pt, dt, activeConfig.smoothingPercent);
    petCompanion_->RequestAction(pet::PetAction::Follow, 0.08);
    petCompanion_->Tick(BuildPetFrameInput(runtimePt, holdButtonDown_, petDragging_, dt));
    UpdatePetVisualState(runtimePt, static_cast<int>(pet::PetAction::Follow), 0.60f);
}

void AppController::DispatchPetButtonDown(const ScreenPoint& pt, int button) {
    if (!config_.mouseCompanion.enabled || !petCompanion_) {
        return;
    }
    const MouseCompanionConfig activeConfig = ResolveActiveMouseCompanionConfig(config_.mouseCompanion);
    petReleaseHoldUntilTickMs_ = 0;
    petLastDispatchPoint_ = pt;
    petHasLastDispatchPoint_ = true;
    const bool primary = (button == static_cast<int>(MouseButton::Left));
    if (primary) {
        petDragging_ = true;
        petCompanion_->RequestAction(pet::PetAction::Drag, 0.03);
    }
    const uint64_t nowTickMs = CurrentTickMs();
    const double dt = ResolvePetDeltaSeconds(&petLastTickMs_, nowTickMs);
    const ScreenPoint runtimePt = ResolvePetRuntimeCursorPoint(pt, dt, activeConfig.smoothingPercent);
    petCompanion_->Tick(BuildPetFrameInput(runtimePt, primary, petDragging_, dt));
    UpdatePetVisualState(
        runtimePt,
        petDragging_ ? static_cast<int>(pet::PetAction::Drag) : static_cast<int>(pet::PetAction::Follow),
        petDragging_ ? 0.8f : 0.6f);
}

void AppController::DispatchPetHoldStart(const ScreenPoint& pt, int button, uint32_t holdMs) {
    if (!config_.mouseCompanion.enabled || !petCompanion_) {
        return;
    }
    const MouseCompanionConfig activeConfig = ResolveActiveMouseCompanionConfig(config_.mouseCompanion);
    petLastDispatchPoint_ = pt;
    petHasLastDispatchPoint_ = true;
    const bool primary = (button == static_cast<int>(MouseButton::Left));
    const uint64_t nowTickMs = CurrentTickMs();
    const double dt = ResolvePetDeltaSeconds(&petLastTickMs_, nowTickMs);
    const ScreenPoint runtimePt = ResolvePetRuntimeCursorPoint(pt, dt, activeConfig.smoothingPercent);
    petCompanion_->RequestAction(pet::PetAction::HoldReact, 0.08);
    petCompanion_->Tick(BuildPetFrameInput(runtimePt, primary, petDragging_, dt));
    const float holdPhase = std::clamp(static_cast<float>(holdMs) / 900.0f, 0.0f, 1.0f);
    const float intensity = 0.62f + holdPhase * 0.28f;
    UpdatePetVisualState(runtimePt, static_cast<int>(pet::PetAction::HoldReact), intensity);
}

void AppController::DispatchPetHoldUpdate(const ScreenPoint& pt, uint32_t holdMs) {
    if (!config_.mouseCompanion.enabled || !petCompanion_) {
        return;
    }
    const MouseCompanionConfig activeConfig = ResolveActiveMouseCompanionConfig(config_.mouseCompanion);
    petLastDispatchPoint_ = pt;
    petHasLastDispatchPoint_ = true;
    const uint64_t nowTickMs = CurrentTickMs();
    const double dt = ResolvePetDeltaSeconds(&petLastTickMs_, nowTickMs);
    const ScreenPoint runtimePt = ResolvePetRuntimeCursorPoint(pt, dt, activeConfig.smoothingPercent);
    petCompanion_->RequestAction(pet::PetAction::HoldReact, 0.06);
    petCompanion_->Tick(BuildPetFrameInput(runtimePt, true, petDragging_, dt));
    const float holdPhase = std::clamp(static_cast<float>(holdMs) / 1200.0f, 0.0f, 1.0f);
    const float intensity = 0.66f + holdPhase * 0.26f;
    UpdatePetVisualState(runtimePt, static_cast<int>(pet::PetAction::HoldReact), intensity);
}

void AppController::DispatchPetButtonUp(const ScreenPoint& pt, int button) {
    if (!config_.mouseCompanion.enabled || !petCompanion_) {
        return;
    }
    const MouseCompanionConfig activeConfig = ResolveActiveMouseCompanionConfig(config_.mouseCompanion);
    petLastDispatchPoint_ = pt;
    petHasLastDispatchPoint_ = true;
    const bool primary = (button == static_cast<int>(MouseButton::Left));
    const uint64_t nowTickMs = CurrentTickMs();
    if (primary) {
        petDragging_ = false;
        if (activeConfig.releaseHoldMs > 0) {
            petReleaseHoldUntilTickMs_ = nowTickMs + static_cast<uint64_t>(activeConfig.releaseHoldMs);
            petCompanion_->RequestAction(pet::PetAction::Drag, 0.06);
        } else {
            petReleaseHoldUntilTickMs_ = 0;
            petCompanion_->RequestAction(pet::PetAction::Follow, 0.08);
        }
    }
    const bool releaseHoldActive =
        primary &&
        activeConfig.releaseHoldMs > 0 &&
        (petReleaseHoldUntilTickMs_ > nowTickMs);
    const double dt = ResolvePetDeltaSeconds(&petLastTickMs_, nowTickMs);
    const ScreenPoint runtimePt = ResolvePetRuntimeCursorPoint(pt, dt, activeConfig.smoothingPercent);
    petCompanion_->Tick(BuildPetFrameInput(runtimePt, false, releaseHoldActive, dt));
    UpdatePetVisualState(
        runtimePt,
        releaseHoldActive ? static_cast<int>(pet::PetAction::Drag) : static_cast<int>(pet::PetAction::Follow),
        releaseHoldActive ? 0.72f : 0.6f);
}

void AppController::DispatchPetHoldEnd(const ScreenPoint& pt) {
    if (!config_.mouseCompanion.enabled || !petCompanion_) {
        return;
    }
    const MouseCompanionConfig activeConfig = ResolveActiveMouseCompanionConfig(config_.mouseCompanion);
    petLastDispatchPoint_ = pt;
    petHasLastDispatchPoint_ = true;
    const uint64_t nowTickMs = CurrentTickMs();
    const double dt = ResolvePetDeltaSeconds(&petLastTickMs_, nowTickMs);
    const ScreenPoint runtimePt = ResolvePetRuntimeCursorPoint(pt, dt, activeConfig.smoothingPercent);
    petCompanion_->RequestAction(pet::PetAction::Follow, 0.09);
    petCompanion_->Tick(BuildPetFrameInput(runtimePt, false, petDragging_, dt));
    UpdatePetVisualState(runtimePt, static_cast<int>(pet::PetAction::Follow), 0.60f);
}

ScreenPoint AppController::ResolvePetRuntimeCursorPoint(
    const ScreenPoint& rawPt,
    double dtSeconds,
    int smoothingPercent) {
    const double alpha = ResolvePetSmoothingAlpha(smoothingPercent, dtSeconds);
    const double rawX = static_cast<double>(rawPt.x);
    const double rawY = static_cast<double>(rawPt.y);

    if (!petHasSmoothedCursor_) {
        petSmoothedCursorX_ = rawX;
        petSmoothedCursorY_ = rawY;
        petHasSmoothedCursor_ = true;
    } else {
        petSmoothedCursorX_ += (rawX - petSmoothedCursorX_) * alpha;
        petSmoothedCursorY_ += (rawY - petSmoothedCursorY_) * alpha;
    }

    ScreenPoint smoothed{};
    smoothed.x = static_cast<long>(std::lround(petSmoothedCursorX_));
    smoothed.y = static_cast<long>(std::lround(petSmoothedCursorY_));
    return smoothed;
}

void AppController::ResetPetDispatchRuntimeState() {
    petDragging_ = false;
    petLastTickMs_ = 0;
    petReleaseHoldUntilTickMs_ = 0;
    petHasSmoothedCursor_ = false;
    petSmoothedCursorX_ = 0.0;
    petSmoothedCursorY_ = 0.0;
    petHasLastDispatchPoint_ = false;
    petLastDispatchPoint_ = ScreenPoint{};
    {
        std::lock_guard<std::mutex> guard(mouseCompanionRuntimeStatusMutex_);
        mouseCompanionRuntimeStatus_.lastActionCode = static_cast<int>(pet::PetAction::Idle);
        mouseCompanionRuntimeStatus_.lastActionIntensity = 0.0f;
        mouseCompanionRuntimeStatus_.lastActionTickMs = CurrentTickMs();
        mouseCompanionRuntimeStatus_.lastActionName = "idle";
    }
}

void AppController::UpdatePetVisualState(const ScreenPoint& pt, int actionCode, float actionIntensity) {
    {
        std::lock_guard<std::mutex> guard(mouseCompanionRuntimeStatusMutex_);
        mouseCompanionRuntimeStatus_.lastActionCode = actionCode;
        mouseCompanionRuntimeStatus_.lastActionIntensity = actionIntensity;
        mouseCompanionRuntimeStatus_.lastActionTickMs = CurrentTickMs();
        mouseCompanionRuntimeStatus_.lastActionName = ResolveMouseCompanionActionName(actionCode);
    }
#if MFX_PLATFORM_MACOS
    if (!config_.mouseCompanion.enabled || petVisualHostHandle_ == nullptr) {
        return;
    }
    ScreenPoint visualPt = pt;
    ScreenPoint cocoaPt{};
    if (macos_overlay_coord_conversion::TryConvertQuartzToCocoa(pt, &cocoaPt)) {
        visualPt = cocoaPt;
    }
    int boneCount = 0;
    if (petCompanion_) {
        if (const auto* skeleton = petCompanion_->CurrentSkeleton()) {
            boneCount = static_cast<int>(skeleton->bones.size());
        }
    }
    mfx_macos_mouse_companion_update_state_v1(
        petVisualHostHandle_,
        visualPt.x,
        visualPt.y,
        actionCode,
        actionIntensity,
        boneCount);

    if (!petCompanion_) {
        return;
    }
    if (!EnsurePetVisualPoseBinding()) {
        return;
    }
    PetVisualPosePayload payload = BuildPetVisualPosePayload(petCompanion_.get());
    if (payload.Count() <= 0) {
        return;
    }
    mfx_macos_mouse_companion_apply_pose_v1(
        petVisualHostHandle_,
        payload.boneIndices.data(),
        payload.positions.data(),
        payload.rotations.data(),
        payload.scales.data(),
        payload.Count());
#else
    (void)pt;
    (void)actionCode;
    (void)actionIntensity;
#endif
}

bool AppController::EnsurePetVisualPoseBinding() {
#if MFX_PLATFORM_MACOS
    if (petVisualHostHandle_ == nullptr || !petCompanion_) {
        std::lock_guard<std::mutex> guard(mouseCompanionRuntimeStatusMutex_);
        mouseCompanionRuntimeStatus_.poseBindingConfigured = false;
        return false;
    }
    if (petVisualPoseBindingConfigured_) {
        return true;
    }
    const pet::SkeletonDesc* skeleton = petCompanion_->CurrentSkeleton();
    if (!skeleton || skeleton->bones.empty()) {
        std::lock_guard<std::mutex> guard(mouseCompanionRuntimeStatusMutex_);
        mouseCompanionRuntimeStatus_.poseBindingConfigured = false;
        mouseCompanionRuntimeStatus_.skeletonBoneCount = 0;
        return false;
    }

    petVisualSkeletonNames_.clear();
    petVisualSkeletonNamePtrs_.clear();
    petVisualSkeletonNames_.reserve(skeleton->bones.size());
    petVisualSkeletonNamePtrs_.reserve(skeleton->bones.size());
    for (size_t i = 0; i < skeleton->bones.size(); ++i) {
        std::string name = skeleton->bones[i].name;
        if (name.empty()) {
            name = "joint_" + std::to_string(i);
        }
        petVisualSkeletonNames_.push_back(std::move(name));
        petVisualSkeletonNamePtrs_.push_back(petVisualSkeletonNames_.back().c_str());
    }

    const int configured = mfx_macos_mouse_companion_configure_pose_binding_v1(
        petVisualHostHandle_,
        petVisualSkeletonNamePtrs_.data(),
        static_cast<int>(petVisualSkeletonNamePtrs_.size()));
    petVisualPoseBindingConfigured_ = (configured != 0);
    {
        std::lock_guard<std::mutex> guard(mouseCompanionRuntimeStatusMutex_);
        mouseCompanionRuntimeStatus_.poseBindingConfigured = petVisualPoseBindingConfigured_;
        mouseCompanionRuntimeStatus_.skeletonBoneCount = static_cast<int>(skeleton->bones.size());
    }
    return petVisualPoseBindingConfigured_;
#else
    return false;
#endif
}

void AppController::TryApplyPetAppearanceToVisualHost() {
#if MFX_PLATFORM_MACOS
    if (petVisualHostHandle_ == nullptr || !petCompanion_) {
        return;
    }
    const pet::AppearanceOverrides& appearance = petCompanion_->CurrentAppearance();

    std::vector<const char*> accessoryIds;
    accessoryIds.reserve(appearance.enabledAccessoryIds.size());
    for (const auto& id : appearance.enabledAccessoryIds) {
        if (!id.empty()) {
            accessoryIds.push_back(id.c_str());
        }
    }

    std::vector<const char*> textureOverrides;
    textureOverrides.reserve(appearance.textureOverridePaths.size());
    for (const auto& value : appearance.textureOverridePaths) {
        if (!value.empty()) {
            textureOverrides.push_back(value.c_str());
        }
    }

    const char* skinVariant = appearance.skinVariantId.empty() ? nullptr : appearance.skinVariantId.c_str();
    (void)mfx_macos_mouse_companion_apply_appearance_v1(
        petVisualHostHandle_,
        skinVariant,
        accessoryIds.empty() ? nullptr : accessoryIds.data(),
        static_cast<int>(accessoryIds.size()),
        textureOverrides.empty() ? nullptr : textureOverrides.data(),
        static_cast<int>(textureOverrides.size()));
#endif
}

void AppController::RememberLastPointerPoint(const ScreenPoint& pt) {
    lastPointerPoint_ = pt;
    hasLastPointerPoint_ = true;
}

bool AppController::TryGetLastPointerPoint(ScreenPoint* outPt) const {
    if (!outPt || !hasLastPointerPoint_) {
        return false;
    }
    *outPt = lastPointerPoint_;
    return true;
}

uint32_t AppController::ResolveWasmFrameTimerIntervalMs() const {
    ScreenPoint pt{};
    if (!QueryCursorScreenPoint(&pt) && !TryGetLastPointerPoint(&pt)) {
        pt.x = 0;
        pt.y = 0;
    }

    int intervalMs = 16;
#if MFX_PLATFORM_MACOS
    intervalMs = macos_overlay_support::ResolveOverlayTimerIntervalMs(pt);
#elif MFX_PLATFORM_WINDOWS
    intervalMs = win32_overlay_timer_support::ResolveTimerIntervalMsForScreenPoint(pt.x, pt.y);
#else
    const int targetFps = (config_.overlayTargetFps > 0) ? config_.overlayTargetFps : 60;
    intervalMs = static_cast<int>(
        std::lround(1000.0 / static_cast<double>(std::max(1, targetFps))));
#endif
    return static_cast<uint32_t>(std::clamp(intervalMs, 4, 1000));
}

std::string AppController::CurrentForegroundProcessBaseName() {
    if (!foregroundProcessService_) {
        return {};
    }
    return foregroundProcessService_->CurrentProcessBaseName();
}

bool AppController::InjectShortcutForTest(const std::string& chordText) {
    if (!keyboardInjector_) {
        return false;
    }
    return keyboardInjector_->SendChord(chordText);
}

void AppController::KillDispatchTimer(uintptr_t timerId) {
    if (!dispatchMessageHost_ || !dispatchMessageHost_->IsCreated()) {
        return;
    }
    dispatchMessageHost_->KillTimer(timerId);
}

} // namespace mousefx
