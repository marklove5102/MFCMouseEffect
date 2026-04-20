#pragma once

#include "MouseFx/Core/Automation/BindingMatchUtils.h"
#include "MouseFx/Core/Config/EffectConfig.h"
#include "MouseFx/Core/System/IForegroundProcessService.h"
#include "MouseFx/Core/System/IKeyboardInjector.h"

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

namespace mousefx::automation_dispatch {

struct DispatchTrace final {
    bool actionAccepted = false;
    bool bindingMatched = false;
    bool injected = false;
    size_t bindingIndex = static_cast<size_t>(-1);
    size_t chainLength = 0;
    int scopeSpecificity = -1;
    std::string normalizedActionId{};
};

bool DispatchBindingActions(
    const AutomationKeyBinding& binding,
    IKeyboardInjector* keyboardInjector);

using BindingActionDispatcher = std::function<bool(const AutomationKeyBinding&)>;

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
    DispatchTrace* outTrace = nullptr);

} // namespace mousefx::automation_dispatch
