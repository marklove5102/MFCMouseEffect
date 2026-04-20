#include "pch.h"
#include "InputAutomationDispatch.h"

#include "MouseFx/Utils/StringUtils.h"

#include <algorithm>
#include <chrono>
#include <thread>

namespace mousefx::automation_dispatch {
namespace {

void AppendActionHistory(
    std::vector<automation_match::ActionHistoryEntry>* history,
    const std::string& actionId,
    size_t cap,
    const automation_match::ChainTimingLimit& timingLimit) {
    if (!history || actionId.empty()) {
        return;
    }

    const auto now = std::chrono::steady_clock::now();
    const size_t targetCap = std::max<size_t>(1, cap);
    history->push_back(automation_match::ActionHistoryEntry{ actionId, now });
    while (history->size() > targetCap) {
        history->erase(history->begin());
    }
    if (timingLimit.maxTotalInterval.count() > 0) {
        const auto oldestAllowed = now - timingLimit.maxTotalInterval;
        while (history->size() > 1 && history->front().timestamp < oldestAllowed) {
            history->erase(history->begin());
        }
    }
}

} // namespace

bool DispatchBindingActions(
    const AutomationKeyBinding& binding,
    IKeyboardInjector* keyboardInjector) {
    if (!keyboardInjector) {
        return false;
    }

    bool executed = false;
    for (const AutomationAction& action : binding.actions) {
        const std::string type = ToLowerAscii(TrimAscii(action.type));
        if (type == "delay") {
            if (action.delayMs == 0) {
                return false;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(action.delayMs));
            continue;
        }

        if (type != "send_shortcut") {
            return false;
        }
        const std::string shortcut = TrimAscii(action.shortcut);
        if (shortcut.empty() || !keyboardInjector->SendChord(shortcut)) {
            return false;
        }

        executed = true;
    }
    return executed;
}

bool DispatchAction(
    const std::vector<AutomationKeyBinding>& mappings,
    std::vector<automation_match::ActionHistoryEntry>* history,
    size_t historyCap,
    const automation_match::ChainTimingLimit& timingLimit,
    const std::string& rawActionId,
    const InputModifierState& modifiers,
    automation_match::NormalizeActionIdFn normalizeActionId,
    IForegroundProcessService* foregroundProcessService,
    const BindingActionDispatcher& dispatchBindingActions,
    DispatchTrace* outTrace) {
    if (outTrace) {
        *outTrace = DispatchTrace{};
    }
    if (!dispatchBindingActions || rawActionId.empty() || !normalizeActionId) {
        return false;
    }

    const std::string normalizedActionId = normalizeActionId(rawActionId);
    if (normalizedActionId.empty()) {
        return false;
    }
    if (outTrace) {
        outTrace->actionAccepted = true;
        outTrace->normalizedActionId = normalizedActionId;
    }

    AppendActionHistory(history, normalizedActionId, historyCap, timingLimit);
    const std::string processBaseName = foregroundProcessService
        ? foregroundProcessService->CurrentProcessBaseName()
        : std::string{};
    const automation_match::BindingMatchResult match = automation_match::FindBestEnabledBinding(
        mappings,
        *history,
        processBaseName,
        timingLimit,
        modifiers,
        normalizeActionId);
    if (!match.binding) {
        return false;
    }
    if (outTrace) {
        outTrace->bindingMatched = true;
        outTrace->bindingIndex = match.bindingIndex;
        outTrace->chainLength = match.chainLength;
        outTrace->scopeSpecificity = match.scopeSpecificity;
    }

    const bool injected = dispatchBindingActions ? dispatchBindingActions(*match.binding) : false;
    if (outTrace) {
        outTrace->injected = injected;
    }
    return injected;
}

} // namespace mousefx::automation_dispatch
