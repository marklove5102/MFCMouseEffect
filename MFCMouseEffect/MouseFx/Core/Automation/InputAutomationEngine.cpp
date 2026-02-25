#include "pch.h"
#include "InputAutomationEngine.h"

#include "MouseFx/Core/Automation/AutomationActionIdNormalizer.h"
#include "MouseFx/Core/Automation/InputAutomationDispatch.h"
#include "MouseFx/Core/Automation/TriggerChainUtils.h"

#include <algorithm>

namespace mousefx {
namespace {

constexpr std::chrono::milliseconds kMouseChainMaxStepIntervalMs(900);
constexpr std::chrono::milliseconds kMouseChainMaxTotalIntervalMs(1800);
constexpr std::chrono::milliseconds kGestureChainMaxStepIntervalMs(2200);
constexpr std::chrono::milliseconds kGestureChainMaxTotalIntervalMs(5000);

GestureRecognitionConfig BuildGestureConfig(const InputAutomationConfig& config) {
    GestureRecognitionConfig out;
    out.enabled = config.enabled && config.gesture.enabled;
    out.triggerButton = config.gesture.triggerButton;
    out.minStrokeDistancePx = config.gesture.minStrokeDistancePx;
    out.sampleStepPx = config.gesture.sampleStepPx;
    out.maxDirections = config.gesture.maxDirections;
    return out;
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

    config_ = config;
    gestureRecognizer_.UpdateConfig(BuildGestureConfig(config_));
    suppressNextClickActionId_.clear();
    mouseActionHistory_.clear();
    gestureHistory_.clear();
    mouseChainCap_ = maxChainLengthForMappings(config_.mouseMappings, false);
    gestureChainCap_ = maxChainLengthForMappings(config_.gesture.mappings, true);
    mouseChainTimingLimit_ = BuildMouseChainTimingLimit();
    gestureChainTimingLimit_ = BuildGestureChainTimingLimit();
}

void InputAutomationEngine::Reset() {
    gestureRecognizer_.Reset();
    suppressNextClickActionId_.clear();
    mouseActionHistory_.clear();
    gestureHistory_.clear();
}

void InputAutomationEngine::OnMouseMove(const ScreenPoint& pt) {
    gestureRecognizer_.OnMouseMove(pt);
}

void InputAutomationEngine::OnButtonDown(const ScreenPoint& pt, int button) {
    gestureRecognizer_.OnButtonDown(pt, button);
}

void InputAutomationEngine::OnButtonUp(const ScreenPoint& pt, int button) {
    const std::string gestureId = gestureRecognizer_.OnButtonUp(pt, button);
    if (!gestureId.empty()) {
        if (TriggerGesture(gestureId)) {
            suppressNextClickActionId_ = automation_ids::NormalizeMouseActionId(ClickActionIdFromButtonCode(button));
        }
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

void InputAutomationEngine::SetForegroundProcessService(IForegroundProcessService* service) {
    foregroundProcessService_ = service;
}

void InputAutomationEngine::SetKeyboardInjector(IKeyboardInjector* injector) {
    keyboardInjector_ = injector;
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
    if (button == 1) return "left_click";
    if (button == 2) return "right_click";
    if (button == 3) return "middle_click";
    return {};
}

std::string InputAutomationEngine::ScrollActionId(short delta) {
    if (delta > 0) return "scroll_up";
    if (delta < 0) return "scroll_down";
    return {};
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
    return automation_dispatch::DispatchAction(
        config_.mouseMappings,
        &mouseActionHistory_,
        mouseChainCap_,
        mouseChainTimingLimit_,
        actionId,
        automation_ids::NormalizeMouseActionId,
        foregroundProcessService_,
        keyboardInjector_);
}

bool InputAutomationEngine::TriggerGesture(const std::string& gestureId) {
    if (!config_.enabled || !config_.gesture.enabled || gestureId.empty()) {
        return false;
    }
    return automation_dispatch::DispatchAction(
        config_.gesture.mappings,
        &gestureHistory_,
        gestureChainCap_,
        gestureChainTimingLimit_,
        gestureId,
        automation_ids::NormalizeGestureId,
        foregroundProcessService_,
        keyboardInjector_);
}

} // namespace mousefx
