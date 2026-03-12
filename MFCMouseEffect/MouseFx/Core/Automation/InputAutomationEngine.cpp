#include "pch.h"
#include "InputAutomationEngine.h"

#include "MouseFx/Core/Automation/AutomationActionIdNormalizer.h"
#include "MouseFx/Core/Automation/InputAutomationDispatch.h"
#include "MouseFx/Core/Automation/TriggerChainUtils.h"
#include "MouseFx/Core/Automation/AppScopeUtils.h"
#include "MouseFx/Core/Input/GestureSimilarity.h"
#include "MouseFx/Utils/StringUtils.h"

#include <algorithm>
#include <cmath>

namespace mousefx {
namespace {

constexpr std::chrono::milliseconds kMouseChainMaxStepIntervalMs(900);
constexpr std::chrono::milliseconds kMouseChainMaxTotalIntervalMs(1800);
constexpr std::chrono::milliseconds kGestureChainMaxStepIntervalMs(2200);
constexpr std::chrono::milliseconds kGestureChainMaxTotalIntervalMs(5000);
constexpr std::chrono::milliseconds kButtonlessArmIdleMs(180);
constexpr std::chrono::milliseconds kButtonlessIdleResetMs(320);
constexpr int kButtonlessThresholdBoostPercent = 6;
constexpr double kButtonlessRunnerUpMarginPercent = 8.0;
constexpr double kButtonlessMinPathLengthFactor = 1.35;
constexpr size_t kButtonlessMinSamplePointCount = 5;
constexpr int kButtonNone = 0;
constexpr int kButtonLeft = 1;
constexpr int kButtonRight = 2;
constexpr int kButtonMiddle = 3;

GestureRecognitionConfig BuildGestureConfig(const InputAutomationConfig& config) {
    GestureRecognitionConfig out;
    out.enabled = config.enabled && config.gesture.enabled;
    out.minStrokeDistancePx = config.gesture.minStrokeDistancePx;
    out.sampleStepPx = config.gesture.sampleStepPx;
    out.maxDirections = config.gesture.maxDirections;
    return out;
}

std::string ButtonNameFromCode(int button) {
    if (button == kButtonNone) return "none";
    if (button == kButtonLeft) return "left";
    if (button == kButtonRight) return "right";
    if (button == kButtonMiddle) return "middle";
    return {};
}

bool IsCustomGestureMode(const AutomationKeyBinding& binding) {
    return IsCustomGesturePatternMode(binding.gesturePattern.mode);
}

std::string NormalizedGestureTailActionId(const AutomationKeyBinding& binding) {
    const std::vector<std::string> chain =
        automation_chain::NormalizeChainTokens(binding.trigger, automation_ids::NormalizeGestureId);
    if (chain.empty()) {
        return {};
    }
    return chain.back();
}

struct ButtonlessDispatchGuard final {
    bool accepted = true;
    const char* reason = "buttonless_candidate_ready";
};

ButtonlessDispatchGuard EvaluateButtonlessGestureGuard(
    const InputAutomationConfig& config,
    const GestureSimilarityMetrics& metrics,
    double bestScore,
    double runnerUpScore,
    int thresholdPercent) {
    const double minPathLengthPx = std::max(
        static_cast<double>(std::max(config.gesture.minStrokeDistancePx, 1)) * kButtonlessMinPathLengthFactor,
        96.0);
    if (metrics.pointCount < kButtonlessMinSamplePointCount ||
        metrics.pathLengthPx + 1e-6 < minPathLengthPx) {
        return ButtonlessDispatchGuard{false, "buttonless_candidate_too_short"};
    }

    const int guardedThreshold = std::clamp(
        thresholdPercent + kButtonlessThresholdBoostPercent,
        50,
        98);
    if (bestScore + 1e-6 < static_cast<double>(guardedThreshold)) {
        return ButtonlessDispatchGuard{false, "buttonless_candidate_below_guard_threshold"};
    }
    if (runnerUpScore >= 0.0 &&
        bestScore - runnerUpScore < kButtonlessRunnerUpMarginPercent) {
        return ButtonlessDispatchGuard{false, "buttonless_candidate_ambiguous"};
    }
    return {};
}

} // namespace

void InputAutomationEngine::UpdateConfig(const InputAutomationConfig& config) {
    auto maxChainLengthForMappings = [](const std::vector<AutomationKeyBinding>& mappings, bool gestureBinding) {
        size_t maxLength = 1;
        for (const AutomationKeyBinding& binding : mappings) {
            if (!binding.enabled) {
                continue;
            }

            const size_t chainLength = gestureBinding
                ? automation_chain::NormalizedChainLength(binding.trigger, automation_ids::NormalizeGestureId)
                : automation_chain::NormalizedChainLength(binding.trigger, automation_ids::NormalizeMouseActionId);
            if (chainLength > maxLength) {
                maxLength = chainLength;
            }
        }
        return std::max<size_t>(1, maxLength);
    };
    auto maxCustomStrokeCountForMappings = [](const std::vector<AutomationKeyBinding>& mappings) {
        size_t maxStrokeCount = 1;
        for (const AutomationKeyBinding& binding : mappings) {
            if (!binding.enabled || !IsCustomGestureMode(binding)) {
                continue;
            }
            const std::vector<std::vector<AutomationKeyBinding::GesturePoint>> templateStrokes =
                GestureTemplateStrokesFromPattern(binding.gesturePattern);
            if (!templateStrokes.empty()) {
                maxStrokeCount = std::max(maxStrokeCount, templateStrokes.size());
            }
        }
        return std::max<size_t>(1, maxStrokeCount);
    };

    config_ = config;
    gestureRecognizer_.UpdateConfig(BuildGestureConfig(config_));
    suppressNextClickActionId_.clear();
    mouseActionHistory_.clear();
    gestureHistory_.clear();
    customGestureStrokeHistory_.clear();
    mouseChainCap_ = maxChainLengthForMappings(config_.mouseMappings, false);
    gestureChainCap_ = maxChainLengthForMappings(config_.gesture.mappings, true);
    customGestureStrokeCap_ = maxCustomStrokeCountForMappings(config_.gesture.mappings);
    mouseChainTimingLimit_ = BuildMouseChainTimingLimit();
    gestureChainTimingLimit_ = BuildGestureChainTimingLimit();
    leftButtonDown_ = false;
    rightButtonDown_ = false;
    middleButtonDown_ = false;
    UpdateButtonlessGestureConfig();
    SetDiagnosticsConfigSnapshot();
    UpdateGestureDiagnostics(
        "config_updated",
        "ready",
        {},
        buttonlessGestureEnabled_ ? "none" : "",
        false,
        false,
        false,
        false,
        0);
}

void InputAutomationEngine::Reset() {
    gestureRecognizer_.Reset();
    ResetButtonlessGestureState();
    suppressNextClickActionId_.clear();
    mouseActionHistory_.clear();
    gestureHistory_.clear();
    customGestureStrokeHistory_.clear();
    currentModifiers_ = {};
    leftButtonDown_ = false;
    rightButtonDown_ = false;
    middleButtonDown_ = false;
    UpdateGestureDiagnostics("runtime_reset", "state_cleared", {}, {}, false, false, false, false, 0);
}

void InputAutomationEngine::OnMouseMove(const ScreenPoint& pt) {
    gestureRecognizer_.OnMouseMove(pt);
    HandleButtonlessGestureMove(pt);
}

void InputAutomationEngine::OnButtonDown(const ScreenPoint& pt, int button) {
    SetButtonState(button, true);
    ResetButtonlessGestureState();
    gestureRecognizer_.OnButtonDown(pt, button);
}

void InputAutomationEngine::OnButtonUp(const ScreenPoint& pt, int button) {
    SetButtonState(button, false);
    const GestureRecognizer::Result gesture = gestureRecognizer_.OnButtonUp(pt, button);
    if (gesture.button > 0 && !gesture.samplePoints.empty()) {
        AppendCustomGestureStroke(gesture.button, gesture.samplePoints);
    }
    if (TriggerGesture(
            gesture.gestureId,
            gesture.button,
            gesture.samplePoints.empty() ? nullptr : &gesture.samplePoints)) {
        suppressNextClickActionId_ = automation_ids::NormalizeMouseActionId(ClickActionIdFromButtonCode(button));
    }
}

void InputAutomationEngine::OnClick(const ClickEvent& ev) {
    const std::string actionId = ClickActionId(ev.button);
    if (!suppressNextClickActionId_.empty()) {
        const bool shouldSuppress = (automation_ids::NormalizeMouseActionId(actionId) == suppressNextClickActionId_);
        suppressNextClickActionId_.clear();
        if (shouldSuppress) {
            return;
        }
    }
    TriggerMouseAction(actionId);
}

void InputAutomationEngine::OnScroll(short delta) {
    TriggerMouseAction(ScrollActionId(delta));
}

void InputAutomationEngine::OnKey(const KeyEvent& ev) {
    currentModifiers_ = ModifierStateFromKeyEvent(ev);
    SetDiagnosticsConfigSnapshot();
}

void InputAutomationEngine::SetForegroundProcessService(IForegroundProcessService* service) {
    foregroundProcessService_ = service;
}

void InputAutomationEngine::SetKeyboardInjector(IKeyboardInjector* injector) {
    keyboardInjector_ = injector;
}

void InputAutomationEngine::SetDiagnosticsEnabled(bool enabled) {
    diagnosticsEnabled_.store(enabled, std::memory_order_release);
    if (!enabled) {
        std::lock_guard<std::mutex> lock(diagnosticsMutex_);
        diagnostics_ = Diagnostics{};
        return;
    }
    SetDiagnosticsConfigSnapshot();
    UpdateGestureDiagnostics(
        "diagnostics_enabled",
        "ready",
        {},
        {},
        false,
        false,
        false,
        false,
        0);
}

bool InputAutomationEngine::DiagnosticsEnabled() const {
    return diagnosticsEnabled_.load(std::memory_order_acquire);
}

InputAutomationEngine::Diagnostics InputAutomationEngine::ReadDiagnostics() const {
    if (!DiagnosticsEnabled()) {
        return Diagnostics{};
    }
    std::lock_guard<std::mutex> lock(diagnosticsMutex_);
    return diagnostics_;
}

void InputAutomationEngine::SetButtonState(int button, bool down) {
    if (button == kButtonLeft) {
        leftButtonDown_ = down;
        return;
    }
    if (button == kButtonRight) {
        rightButtonDown_ = down;
        return;
    }
    if (button == kButtonMiddle) {
        middleButtonDown_ = down;
    }
}

bool InputAutomationEngine::AnyPointerButtonDown() const {
    return leftButtonDown_ || rightButtonDown_ || middleButtonDown_;
}

bool InputAutomationEngine::HasNoButtonGestureMappings() const {
    if (!config_.enabled || !config_.gesture.enabled) {
        return false;
    }
    return CountNoButtonGestureMappings() > 0;
}

uint64_t InputAutomationEngine::CountNoButtonGestureMappings() const {
    uint64_t count = 0;
    for (const AutomationKeyBinding& binding : config_.gesture.mappings) {
        if (binding.enabled && binding.triggerButton == "none") {
            ++count;
        }
    }
    return count;
}

void InputAutomationEngine::SetDiagnosticsConfigSnapshot() {
    if (!DiagnosticsEnabled()) {
        return;
    }
    std::lock_guard<std::mutex> lock(diagnosticsMutex_);
    diagnostics_.automationEnabled = config_.enabled;
    diagnostics_.gestureEnabled = config_.gesture.enabled;
    diagnostics_.buttonlessGestureEnabled = buttonlessGestureEnabled_;
    diagnostics_.pointerButtonDown = AnyPointerButtonDown();
    diagnostics_.gestureMappingCount = static_cast<uint64_t>(config_.gesture.mappings.size());
    diagnostics_.buttonlessGestureMappingCount = CountNoButtonGestureMappings();
}

void InputAutomationEngine::UpdateGestureDiagnostics(
    const char* stage,
    const char* reason,
    const std::string& gestureId,
    const std::string& triggerButton,
    bool matched,
    bool injected,
    bool usedCustom,
    bool usedPreset,
    size_t samplePointCount) {
    if (!DiagnosticsEnabled()) {
        return;
    }
    std::lock_guard<std::mutex> lock(diagnosticsMutex_);
    diagnostics_.automationEnabled = config_.enabled;
    diagnostics_.gestureEnabled = config_.gesture.enabled;
    diagnostics_.buttonlessGestureEnabled = buttonlessGestureEnabled_;
    diagnostics_.pointerButtonDown = AnyPointerButtonDown();
    diagnostics_.gestureMappingCount = static_cast<uint64_t>(config_.gesture.mappings.size());
    diagnostics_.buttonlessGestureMappingCount = CountNoButtonGestureMappings();
    diagnostics_.lastStage = stage ? stage : "";
    diagnostics_.lastReason = reason ? reason : "";
    diagnostics_.lastGestureId = gestureId;
    diagnostics_.lastTriggerButton = triggerButton;
    diagnostics_.lastMatched = matched;
    diagnostics_.lastInjected = injected;
    diagnostics_.lastUsedCustom = usedCustom;
    diagnostics_.lastUsedPreset = usedPreset;
    diagnostics_.lastSamplePointCount = static_cast<int>(samplePointCount);
    diagnostics_.lastModifiers = currentModifiers_;
}

void InputAutomationEngine::UpdateButtonlessGestureConfig() {
    buttonlessGestureEnabled_ = HasNoButtonGestureMappings();

    GestureRecognitionConfig buttonlessConfig = BuildGestureConfig(config_);
    buttonlessConfig.enabled = buttonlessGestureEnabled_;
    buttonlessConfig.triggerButton = "none";
    buttonlessGestureRecognizer_.UpdateConfig(buttonlessConfig);
    ResetButtonlessGestureState();
    SetDiagnosticsConfigSnapshot();
}

void InputAutomationEngine::ResetButtonlessGestureState() {
    buttonlessGestureRecognizer_.Reset();
    buttonlessGestureTriggered_ = false;
    buttonlessLastGestureId_.clear();
    buttonlessLastMoveAt_ = {};
    SetDiagnosticsConfigSnapshot();
}

void InputAutomationEngine::HandleButtonlessGestureMove(const ScreenPoint& pt) {
    if (!buttonlessGestureEnabled_ || AnyPointerButtonDown()) {
        if (!buttonlessGestureEnabled_) {
            UpdateGestureDiagnostics(
                "buttonless_move_skipped",
                "buttonless_disabled",
                {},
                "none",
                false,
                false,
                false,
                false,
                0);
        } else {
            UpdateGestureDiagnostics(
                "buttonless_move_skipped",
                "pointer_button_is_down",
                {},
                "none",
                false,
                false,
                false,
                false,
                0);
        }
        ResetButtonlessGestureState();
        return;
    }

    const auto now = std::chrono::steady_clock::now();
    const bool hadPreviousMove = buttonlessLastMoveAt_.time_since_epoch().count() > 0;
    const auto idleGap = hadPreviousMove
        ? now - buttonlessLastMoveAt_
        : std::chrono::steady_clock::duration::zero();
    if (buttonlessGestureRecognizer_.IsActive() &&
        hadPreviousMove &&
        idleGap > kButtonlessIdleResetMs) {
        UpdateGestureDiagnostics(
            "buttonless_idle_reset",
            "idle_timeout",
            {},
            "none",
            false,
            false,
            false,
            false,
            0);
        ResetButtonlessGestureState();
    }

    if (!buttonlessGestureRecognizer_.IsActive()) {
        if (!hadPreviousMove || idleGap < kButtonlessArmIdleMs) {
            UpdateGestureDiagnostics(
                "buttonless_move_skipped",
                hadPreviousMove ? "awaiting_idle_arm" : "awaiting_first_idle_arm",
                {},
                "none",
                false,
                false,
                false,
                false,
                0);
            buttonlessLastMoveAt_ = now;
            return;
        }
        buttonlessGestureRecognizer_.OnButtonDown(pt, kButtonNone);
        UpdateGestureDiagnostics(
            "buttonless_arm",
            "idle_arm_ready",
            {},
            "none",
            false,
            false,
            false,
            false,
            0);
    }
    buttonlessGestureRecognizer_.OnMouseMove(pt);
    buttonlessLastMoveAt_ = now;

    if (buttonlessGestureTriggered_) {
        return;
    }
    const GestureRecognizer::Result snapshot = buttonlessGestureRecognizer_.Snapshot();
    if (snapshot.gestureId.empty()) {
        UpdateGestureDiagnostics(
            "buttonless_snapshot",
            "gesture_id_empty",
            {},
            "none",
            false,
            false,
            false,
            false,
            snapshot.samplePoints.size());
        buttonlessLastGestureId_.clear();
        return;
    }
    if (!buttonlessLastGestureId_.empty() &&
        snapshot.gestureId == buttonlessLastGestureId_) {
        return;
    }
    if (TriggerGesture(
            snapshot.gestureId,
            kButtonNone,
            snapshot.samplePoints.empty() ? nullptr : &snapshot.samplePoints)) {
        buttonlessLastGestureId_ = snapshot.gestureId;
        buttonlessGestureTriggered_ = true;
    } else {
        buttonlessLastGestureId_.clear();
    }
}

std::string InputAutomationEngine::ClickActionId(MouseButton button) {
    switch (button) {
    case MouseButton::Left: return "left_click";
    case MouseButton::Right: return "right_click";
    case MouseButton::Middle: return "middle_click";
    default: break;
    }
    return {};
}

std::string InputAutomationEngine::ClickActionIdFromButtonCode(int button) {
    if (button == kButtonLeft) return "left_click";
    if (button == kButtonRight) return "right_click";
    if (button == kButtonMiddle) return "middle_click";
    return {};
}

std::string InputAutomationEngine::ScrollActionId(short delta) {
    if (delta > 0) return "scroll_up";
    if (delta < 0) return "scroll_down";
    return {};
}

InputModifierState InputAutomationEngine::ModifierStateFromKeyEvent(const KeyEvent& ev) {
    InputModifierState modifiers;
    modifiers.primary = ev.ctrl || ev.meta;
    modifiers.shift = ev.shift;
    modifiers.alt = ev.alt;
    return modifiers;
}

InputAutomationEngine::ChainTimingLimit InputAutomationEngine::BuildMouseChainTimingLimit() {
    ChainTimingLimit limit;
    limit.maxStepInterval = kMouseChainMaxStepIntervalMs;
    limit.maxTotalInterval = kMouseChainMaxTotalIntervalMs;
    return limit;
}

InputAutomationEngine::ChainTimingLimit InputAutomationEngine::BuildGestureChainTimingLimit() {
    ChainTimingLimit limit;
    limit.maxStepInterval = kGestureChainMaxStepIntervalMs;
    limit.maxTotalInterval = kGestureChainMaxTotalIntervalMs;
    return limit;
}

bool InputAutomationEngine::TriggerMouseAction(const std::string& actionId) {
    if (!config_.enabled || actionId.empty()) {
        return false;
    }
    automation_dispatch::DispatchTrace trace{};
    const bool injected = automation_dispatch::DispatchAction(
        config_.mouseMappings,
        &mouseActionHistory_,
        mouseChainCap_,
        mouseChainTimingLimit_,
        actionId,
        InputModifierState{},
        automation_ids::NormalizeMouseActionId,
        foregroundProcessService_,
        keyboardInjector_,
        &trace);
    (void)trace;
    return injected;
}

bool InputAutomationEngine::TriggerGesture(
    const std::string& gestureId,
    int button,
    const std::vector<ScreenPoint>* currentStroke) {
    if (!config_.enabled || !config_.gesture.enabled) {
        UpdateGestureDiagnostics(
            "gesture_trigger",
            !config_.enabled ? "automation_disabled" : "gesture_disabled",
            gestureId,
            {},
            false,
            false,
            false,
            false,
            currentStroke ? currentStroke->size() : 0);
        return false;
    }
    const std::string triggerButton = ButtonNameFromCode(button);
    if (triggerButton.empty()) {
        UpdateGestureDiagnostics(
            "gesture_trigger",
            "unknown_button_code",
            gestureId,
            {},
            false,
            false,
            false,
            false,
            currentStroke ? currentStroke->size() : 0);
        return false;
    }
    if (TriggerCustomGesture(button, triggerButton, currentStroke)) {
        UpdateGestureDiagnostics(
            "gesture_trigger",
            "custom_binding_injected",
            gestureId,
            triggerButton,
            true,
            true,
            true,
            false,
            currentStroke ? currentStroke->size() : 0);
        return true;
    }
    std::vector<AutomationKeyBinding> filteredMappings;
    filteredMappings.reserve(config_.gesture.mappings.size());
    for (const AutomationKeyBinding& binding : config_.gesture.mappings) {
        if (binding.triggerButton == triggerButton && !IsCustomGestureMode(binding)) {
            filteredMappings.push_back(binding);
        }
    }
    if (filteredMappings.empty()) {
        UpdateGestureDiagnostics(
            "gesture_trigger",
            "no_preset_mapping_for_button",
            gestureId,
            triggerButton,
            false,
            false,
            false,
            true,
            currentStroke ? currentStroke->size() : 0);
        return false;
    }

    std::vector<AutomationKeyBinding> dispatchMappings = filteredMappings;
    std::string dispatchGestureId = gestureId;
    bool usedPresetSimilarity = false;
    double bestSimilarityScore = -1.0;
    double runnerUpSimilarityScore = -1.0;
    int bestSimilarityThreshold = 0;
    int bestSimilarityScope = -1;
    std::string similarityRejectReason =
        (currentStroke && !currentStroke->empty()) ? "preset_similarity_not_matched" : "gesture_id_empty";
    const GestureSimilarityMetrics currentStrokeMetrics =
        (currentStroke && !currentStroke->empty())
            ? MeasureCapturedGesture({*currentStroke})
            : GestureSimilarityMetrics{};

    if (currentStroke && !currentStroke->empty()) {
        struct PresetSimilarityCandidate final {
            AutomationKeyBinding binding;
            std::string actionId;
            double score = -1.0;
            int threshold = 75;
            int scopeSpecificity = -1;
        };

        std::vector<PresetSimilarityCandidate> candidates;
        candidates.reserve(filteredMappings.size());
        std::string bestActionId;

        for (const AutomationKeyBinding& binding : filteredMappings) {
            if (!binding.enabled) {
                continue;
            }
            const std::string actionId = NormalizedGestureTailActionId(binding);
            if (actionId.empty()) {
                continue;
            }

            const double score = ScorePresetGestureSimilarity(actionId, *currentStroke);
            if (score < 0.0) {
                continue;
            }
            const int threshold = std::clamp(binding.gesturePattern.matchThresholdPercent, 50, 95);
            if (score + 1e-6 < static_cast<double>(threshold)) {
                continue;
            }

            const int scopeSpecificity = automation_scope::AppScopeSpecificity(binding.appScopes);
            candidates.push_back(PresetSimilarityCandidate{
                binding,
                actionId,
                score,
                threshold,
                scopeSpecificity,
            });

            if (bestActionId.empty() ||
                score > bestSimilarityScore + 1e-6 ||
                (std::abs(score - bestSimilarityScore) <= 1e-6 &&
                 scopeSpecificity > bestSimilarityScope)) {
                bestActionId = actionId;
                bestSimilarityScore = score;
                bestSimilarityThreshold = threshold;
                bestSimilarityScope = scopeSpecificity;
            }
        }

        if (!bestActionId.empty()) {
            dispatchMappings.clear();
            for (const PresetSimilarityCandidate& candidate : candidates) {
                if (candidate.actionId == bestActionId) {
                    dispatchMappings.push_back(candidate.binding);
                } else {
                    runnerUpSimilarityScore = std::max(runnerUpSimilarityScore, candidate.score);
                }
            }
            if (triggerButton == "none") {
                const ButtonlessDispatchGuard guard = EvaluateButtonlessGestureGuard(
                    config_,
                    currentStrokeMetrics,
                    bestSimilarityScore,
                    runnerUpSimilarityScore,
                    bestSimilarityThreshold);
                if (!guard.accepted) {
                    similarityRejectReason = guard.reason;
                    dispatchMappings.clear();
                    dispatchGestureId.clear();
                    bestActionId.clear();
                }
            }
        }

        if (!bestActionId.empty()) {
            dispatchGestureId = bestActionId;
            usedPresetSimilarity = true;
        }
    }

    if (dispatchGestureId.empty()) {
        UpdateGestureDiagnostics(
            "gesture_trigger",
            similarityRejectReason.c_str(),
            {},
            triggerButton,
            false,
            false,
            false,
            true,
            currentStroke ? currentStroke->size() : 0);
        return false;
    }

    automation_dispatch::DispatchTrace trace{};
    const bool injected = automation_dispatch::DispatchAction(
        dispatchMappings,
        &gestureHistory_,
        gestureChainCap_,
        gestureChainTimingLimit_,
        dispatchGestureId,
        currentModifiers_,
        automation_ids::NormalizeGestureId,
        foregroundProcessService_,
        keyboardInjector_,
        &trace);
    std::string reason = "preset_binding_not_matched";
    if (usedPresetSimilarity) {
        reason = "preset_similarity_binding_not_matched";
        if (!trace.actionAccepted) {
            reason = "preset_similarity_action_not_accepted";
        } else if (trace.bindingMatched && !injected) {
            reason = "preset_similarity_binding_matched_but_inject_failed";
        } else if (injected) {
            reason = "preset_similarity_binding_injected";
        }
        (void)bestSimilarityScore;
        (void)bestSimilarityThreshold;
    } else {
        if (!trace.actionAccepted) {
            reason = "preset_action_not_accepted";
        } else if (trace.bindingMatched && !injected) {
            reason = "preset_binding_matched_but_inject_failed";
        } else if (injected) {
            reason = "preset_binding_injected";
        }
    }
    const std::string reportedGestureId = usedPresetSimilarity ? dispatchGestureId : gestureId;
    UpdateGestureDiagnostics(
        "gesture_trigger",
        reason.c_str(),
        reportedGestureId,
        triggerButton,
        trace.bindingMatched,
        injected,
        false,
        true,
        currentStroke ? currentStroke->size() : 0);
    return injected;
}

bool InputAutomationEngine::TriggerCustomGesture(
    int button,
    const std::string& triggerButton,
    const std::vector<ScreenPoint>* currentStroke) {
    if (!keyboardInjector_) {
        UpdateGestureDiagnostics(
            "custom_trigger",
            "keyboard_injector_unavailable",
            {},
            triggerButton,
            false,
            false,
            true,
            false,
            currentStroke ? currentStroke->size() : 0);
        return false;
    }

    struct MatchCandidate final {
        const AutomationKeyBinding* binding = nullptr;
        size_t strokeCount = 0;
        double score = -1.0;
        int scopeSpecificity = -1;
        int threshold = 75;
    };

    const std::string processBaseName = foregroundProcessService_
        ? foregroundProcessService_->CurrentProcessBaseName()
        : std::string{};
    std::vector<MatchCandidate> matches;
    matches.reserve(config_.gesture.mappings.size());
    for (const AutomationKeyBinding& binding : config_.gesture.mappings) {
        if (!binding.enabled || binding.triggerButton != triggerButton || !IsCustomGestureMode(binding)) {
            continue;
        }
        if (!automation_scope::AppScopeMatchesProcess(binding.appScopes, processBaseName)) {
            continue;
        }
        if (!automation_match::ModifierConditionMatches(binding.modifiers, currentModifiers_)) {
            continue;
        }

        const std::vector<std::vector<AutomationKeyBinding::GesturePoint>> templateStrokes =
            GestureTemplateStrokesFromPattern(binding.gesturePattern);
        if (templateStrokes.empty()) {
            continue;
        }

        std::vector<std::vector<ScreenPoint>> capturedStrokes;
        std::vector<std::chrono::steady_clock::time_point> timestamps;
        const bool buttonlessCurrentStrokeMode =
            (triggerButton == "none" && currentStroke && !currentStroke->empty());
        if (buttonlessCurrentStrokeMode) {
            // Without mouse-button boundaries, we match the current live stroke.
            if (templateStrokes.size() != 1) {
                continue;
            }
            capturedStrokes.push_back(*currentStroke);
        } else {
            capturedStrokes.reserve(templateStrokes.size());
            timestamps.reserve(templateStrokes.size());
            for (auto it = customGestureStrokeHistory_.rbegin();
                 it != customGestureStrokeHistory_.rend() &&
                 capturedStrokes.size() < templateStrokes.size();
                 ++it) {
                if (it->button == button) {
                    capturedStrokes.push_back(it->points);
                    timestamps.push_back(it->timestamp);
                }
            }
        }
        if (capturedStrokes.size() != templateStrokes.size()) {
            continue;
        }
        std::reverse(capturedStrokes.begin(), capturedStrokes.end());
        std::reverse(timestamps.begin(), timestamps.end());

        if (!timestamps.empty() &&
            (gestureChainTimingLimit_.maxStepInterval.count() > 0 ||
             gestureChainTimingLimit_.maxTotalInterval.count() > 0)) {
            bool timingMatched = true;
            if (gestureChainTimingLimit_.maxStepInterval.count() > 0) {
                for (size_t i = 1; i < timestamps.size(); ++i) {
                    if (timestamps[i] - timestamps[i - 1] > gestureChainTimingLimit_.maxStepInterval) {
                        timingMatched = false;
                        break;
                    }
                }
            }
            if (timingMatched &&
                gestureChainTimingLimit_.maxTotalInterval.count() > 0 &&
                timestamps.size() > 1 &&
                timestamps.back() - timestamps.front() > gestureChainTimingLimit_.maxTotalInterval) {
                timingMatched = false;
            }
            if (!timingMatched) {
                continue;
            }
        }

        const double score = ScoreGestureTemplateSimilarity(templateStrokes, capturedStrokes);
        if (score < 0.0) {
            continue;
        }

        const int threshold = std::clamp(binding.gesturePattern.matchThresholdPercent, 50, 95);
        if (score + 1e-6 < static_cast<double>(threshold)) {
            continue;
        }

        const int scopeSpecificity = automation_scope::AppScopeSpecificity(binding.appScopes);
        matches.push_back(MatchCandidate{
            &binding,
            templateStrokes.size(),
            score,
            scopeSpecificity,
            threshold,
        });
    }

    MatchCandidate best{};
    for (const MatchCandidate& candidate : matches) {
        if (!best.binding ||
            candidate.score > best.score + 1e-6 ||
            (std::abs(candidate.score - best.score) <= 1e-6 &&
             candidate.strokeCount > best.strokeCount) ||
            (std::abs(candidate.score - best.score) <= 1e-6 &&
             candidate.strokeCount == best.strokeCount &&
             candidate.scopeSpecificity > best.scopeSpecificity)) {
            best = candidate;
        }
    }
    if (!best.binding) {
        UpdateGestureDiagnostics(
            "custom_trigger",
            "no_custom_mapping_matched",
            {},
            triggerButton,
            false,
            false,
            true,
            false,
            currentStroke ? currentStroke->size() : 0);
        return false;
    }

    if (triggerButton == "none" && currentStroke && !currentStroke->empty()) {
        double runnerUpScore = -1.0;
        for (const MatchCandidate& candidate : matches) {
            if (candidate.binding != best.binding) {
                runnerUpScore = std::max(runnerUpScore, candidate.score);
            }
        }
        const ButtonlessDispatchGuard guard = EvaluateButtonlessGestureGuard(
            config_,
            MeasureCapturedGesture({*currentStroke}),
            best.score,
            runnerUpScore,
            best.threshold);
        if (!guard.accepted) {
            UpdateGestureDiagnostics(
                "custom_trigger",
                guard.reason,
                {},
                triggerButton,
                false,
                false,
                true,
                false,
                currentStroke ? currentStroke->size() : 0);
            return false;
        }
    }

    const std::string keys = TrimAscii(best.binding->keys);
    if (keys.empty()) {
        UpdateGestureDiagnostics(
            "custom_trigger",
            "custom_mapping_missing_keys",
            {},
            triggerButton,
            true,
            false,
            true,
            false,
            currentStroke ? currentStroke->size() : 0);
        return false;
    }
    if (!keyboardInjector_->SendChord(keys)) {
        UpdateGestureDiagnostics(
            "custom_trigger",
            "custom_mapping_inject_failed",
            {},
            triggerButton,
            true,
            false,
            true,
            false,
            currentStroke ? currentStroke->size() : 0);
        return false;
    }
    if (triggerButton != "none") {
        ConsumeRecentCustomGestureStrokes(button, best.strokeCount);
    }
    return true;
}

void InputAutomationEngine::AppendCustomGestureStroke(
    int button,
    const std::vector<ScreenPoint>& points) {
    if (button <= 0 || points.empty()) {
        return;
    }

    const auto now = std::chrono::steady_clock::now();
    customGestureStrokeHistory_.push_back(CustomGestureStrokeEntry{
        button,
        points,
        now,
    });

    const size_t cap = std::max<size_t>(1, customGestureStrokeCap_);
    while (customGestureStrokeHistory_.size() > cap) {
        customGestureStrokeHistory_.erase(customGestureStrokeHistory_.begin());
    }
    if (gestureChainTimingLimit_.maxTotalInterval.count() > 0) {
        const auto oldestAllowed = now - gestureChainTimingLimit_.maxTotalInterval;
        while (customGestureStrokeHistory_.size() > 1 &&
               customGestureStrokeHistory_.front().timestamp < oldestAllowed) {
            customGestureStrokeHistory_.erase(customGestureStrokeHistory_.begin());
        }
    }
}

void InputAutomationEngine::ConsumeRecentCustomGestureStrokes(int button, size_t count) {
    if (button <= 0 || count == 0 || customGestureStrokeHistory_.empty()) {
        return;
    }

    for (size_t i = customGestureStrokeHistory_.size(); i > 0 && count > 0; --i) {
        const size_t index = i - 1;
        if (customGestureStrokeHistory_[index].button != button) {
            continue;
        }
        customGestureStrokeHistory_.erase(customGestureStrokeHistory_.begin() + static_cast<std::ptrdiff_t>(index));
        --count;
    }
}

} // namespace mousefx
