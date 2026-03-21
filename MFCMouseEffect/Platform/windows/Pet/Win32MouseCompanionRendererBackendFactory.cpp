#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionRendererBackendFactory.h"

#include "Platform/windows/Pet/Win32MouseCompanionRendererBackendRegistry.h"
#include "Platform/windows/Pet/Win32MouseCompanionRendererBackendPreference.h"
#include "Platform/windows/Pet/Win32MouseCompanionPlaceholderRenderer.h"
#include "Platform/windows/Pet/Win32MouseCompanionRealRendererBackend.h"

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
    bool preferredRegistered,
    bool preferredAvailable) {
    if (selectedBackendName.empty()) {
        if (!preferredRegistered) {
            return "preferred_backend_not_registered";
        }
        if (!preferredAvailable) {
            return "preferred_backend_unavailable";
        }
        return "no_backend_available";
    }
    if (preferredBackendName.empty() || IsAutoWin32MouseCompanionRendererBackendName(preferredBackendName)) {
        return "auto_highest_priority_selected";
    }
    if (selectedBackendName == preferredBackendName) {
        return "preferred_backend_selected";
    }
    if (!preferredRegistered) {
        return "preferred_backend_not_registered";
    }
    if (!preferredAvailable) {
        return "preferred_backend_unavailable_fallback_selected";
    }
    return "preferred_backend_fallback_selected";
}

} // namespace

std::string GetEffectiveWin32MouseCompanionRendererBackendPreference() {
    return ResolveWin32MouseCompanionRendererBackendPreference().backendName;
}

Win32MouseCompanionRendererBackendSelection SelectDefaultWin32MouseCompanionRendererBackend(
    const Win32MouseCompanionRendererBackendPreferenceRequest& request) {
    RegisterWin32MouseCompanionRealRendererBackend();
    RegisterWin32MouseCompanionPlaceholderRendererBackend();
    Win32MouseCompanionRendererBackendSelection selection{};
    const auto preference = ResolveWin32MouseCompanionRendererBackendPreference(request);
    selection.preferredBackendSource = preference.source;
    selection.preferredBackendName = preference.backendName;

    auto descriptors = Win32MouseCompanionRendererBackendRegistry::Instance().ListByPriority();
    const bool preferredRegistered =
        selection.preferredBackendName.empty() ||
            IsAutoWin32MouseCompanionRendererBackendName(selection.preferredBackendName)
            ? true
            : std::any_of(descriptors.begin(), descriptors.end(), [&](const auto& descriptor) {
                  return descriptor.name == selection.preferredBackendName;
              });
    const bool preferredAvailable =
        selection.preferredBackendName.empty() ||
            IsAutoWin32MouseCompanionRendererBackendName(selection.preferredBackendName)
            ? true
            : std::any_of(descriptors.begin(), descriptors.end(), [&](const auto& descriptor) {
                  return descriptor.name == selection.preferredBackendName && descriptor.available;
              });
    PrioritizePreferredBackend(&descriptors, selection.preferredBackendName);

    selection.availableBackendNames.reserve(descriptors.size());
    selection.unavailableBackendReasons.reserve(descriptors.size());
    for (const auto& descriptor : descriptors) {
        if (descriptor.available) {
            selection.availableBackendNames.push_back(descriptor.name);
            continue;
        }
        std::string reason = descriptor.name;
        if (!descriptor.unavailableReason.empty()) {
            reason += ":" + descriptor.unavailableReason;
        }
        selection.unavailableBackendReasons.push_back(std::move(reason));
    }

    for (const auto& descriptor : descriptors) {
        if (!descriptor.available) {
            continue;
        }
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
            ComposeSelectionReason(
                selection.preferredBackendName,
                selection.selectedBackendName,
                preferredRegistered,
                preferredAvailable);
        selection.backend = std::move(backend);
        return selection;
    }

    selection.selectedBackendName = "placeholder_fallback";
    selection.selectionReason =
        ComposeSelectionReason(
            selection.preferredBackendName,
            selection.selectedBackendName,
            preferredRegistered,
            preferredAvailable);
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

Win32MouseCompanionRendererBackendSelection SelectDefaultWin32MouseCompanionRendererBackend(
    const std::string& preferredBackendName) {
    if (preferredBackendName.empty()) {
        return SelectDefaultWin32MouseCompanionRendererBackend(Win32MouseCompanionRendererBackendPreferenceRequest{});
    }
    Win32MouseCompanionRendererBackendPreferenceRequest request{};
    request.preferredBackendSource = "explicit_argument";
    request.preferredBackendName = preferredBackendName;
    return SelectDefaultWin32MouseCompanionRendererBackend(request);
}

std::unique_ptr<IWin32MouseCompanionRendererBackend> CreateDefaultWin32MouseCompanionRendererBackend() {
    return SelectDefaultWin32MouseCompanionRendererBackend().backend;
}

} // namespace mousefx::windows
