#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionRendererBackendFactory.h"

#include "Platform/windows/Pet/Win32MouseCompanionRendererBackendRegistry.h"
#include "Platform/windows/Pet/Win32MouseCompanionRendererBackendPreference.h"
#include "Platform/windows/Pet/Win32MouseCompanionPlaceholderRenderer.h"

#include <algorithm>

namespace mousefx::windows {
namespace {

std::string ComposeBackendFailureReason(
    const std::string& backendName,
    const char* category,
    const std::string& detail = {}) {
    std::string reason = std::string(category) + ":" + backendName;
    if (!detail.empty()) {
        reason += ":" + detail;
    }
    return reason;
}

void PrioritizePreferredBackend(
    std::vector<Win32MouseCompanionRendererBackendRegistry::Descriptor>* descriptors,
    const std::string& preferredBackendName) {
    if (!descriptors || preferredBackendName.empty() ||
        IsAutoWin32MouseCompanionRendererBackendName(preferredBackendName)) {
        return;
    }
    auto& list = *descriptors;
    auto it = std::find_if(list.begin(), list.end(), [&](const auto& descriptor) {
        return descriptor.name == preferredBackendName;
    });
    if (it == list.end() || it == list.begin()) {
        return;
    }
    const auto picked = *it;
    list.erase(it);
    list.insert(list.begin(), picked);
}

std::string ComposeSelectionReason(
    const std::string& preferredBackendName,
    const std::string& selectedBackendName,
    bool preferredRegistered) {
    if (selectedBackendName.empty()) {
        return preferredRegistered ? "no_backend_available" : "preferred_backend_not_registered";
    }
    if (preferredBackendName.empty() || IsAutoWin32MouseCompanionRendererBackendName(preferredBackendName)) {
        return "auto_highest_priority_selected";
    }
    if (selectedBackendName == preferredBackendName) {
        return "preferred_backend_selected";
    }
    return preferredRegistered ? "preferred_backend_fallback_selected" : "preferred_backend_not_registered";
}

} // namespace

std::string GetEffectiveWin32MouseCompanionRendererBackendPreference() {
    return ResolveWin32MouseCompanionRendererBackendPreference().backendName;
}

Win32MouseCompanionRendererBackendSelection SelectDefaultWin32MouseCompanionRendererBackend(
    const std::string& preferredBackendName) {
    RegisterWin32MouseCompanionPlaceholderRendererBackend();
    Win32MouseCompanionRendererBackendSelection selection{};
    if (preferredBackendName.empty()) {
        const auto preference = ResolveWin32MouseCompanionRendererBackendPreference();
        selection.preferredBackendSource = preference.source;
        selection.preferredBackendName = preference.backendName;
    } else {
        const auto preference = ResolveExplicitWin32MouseCompanionRendererBackendPreference(preferredBackendName);
        selection.preferredBackendSource = preference.source;
        selection.preferredBackendName = preference.backendName;
    }

    auto descriptors = Win32MouseCompanionRendererBackendRegistry::Instance().ListByPriority();
    const bool preferredRegistered =
        selection.preferredBackendName.empty() ||
            IsAutoWin32MouseCompanionRendererBackendName(selection.preferredBackendName)
            ? true
            : std::any_of(descriptors.begin(), descriptors.end(), [&](const auto& descriptor) {
                  return descriptor.name == selection.preferredBackendName;
              });
    PrioritizePreferredBackend(&descriptors, selection.preferredBackendName);

    selection.availableBackendNames.reserve(descriptors.size());
    for (const auto& descriptor : descriptors) {
        selection.availableBackendNames.push_back(descriptor.name);
    }

    for (const auto& descriptor : descriptors) {
        auto backend = Win32MouseCompanionRendererBackendRegistry::Instance().Create(descriptor.name);
        if (!backend) {
            selection.failureReason = ComposeBackendFailureReason(descriptor.name, "backend_create_failed");
            continue;
        }
        if (!backend->Start()) {
            selection.failureReason = ComposeBackendFailureReason(
                descriptor.name,
                "backend_start_failed",
                backend->LastErrorReason());
            backend->Shutdown();
            continue;
        }
        if (!backend->IsReady()) {
            selection.failureReason = ComposeBackendFailureReason(
                descriptor.name,
                "backend_not_ready",
                backend->LastErrorReason());
            backend->Shutdown();
            continue;
        }
        selection.selectedBackendName = descriptor.name;
        selection.selectionReason =
            ComposeSelectionReason(selection.preferredBackendName, selection.selectedBackendName, preferredRegistered);
        selection.backend = std::move(backend);
        return selection;
    }

    selection.selectedBackendName = "placeholder_fallback";
    selection.selectionReason =
        ComposeSelectionReason(selection.preferredBackendName, selection.selectedBackendName, preferredRegistered);
    selection.backend = std::make_unique<Win32MouseCompanionPlaceholderRenderer>();
    if (selection.backend && !selection.backend->Start()) {
        selection.failureReason = ComposeBackendFailureReason(
            selection.selectedBackendName,
            "backend_start_failed",
            selection.backend->LastErrorReason());
        selection.backend.reset();
    }
    if (selection.availableBackendNames.empty()) {
        selection.availableBackendNames.push_back("placeholder_fallback");
    }
    if (selection.failureReason.empty()) {
        selection.failureReason = "no_registered_renderer_backend";
    }
    return selection;
}

std::unique_ptr<IWin32MouseCompanionRendererBackend> CreateDefaultWin32MouseCompanionRendererBackend() {
    return SelectDefaultWin32MouseCompanionRendererBackend().backend;
}

} // namespace mousefx::windows
