#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionRendererBackendFactory.h"

#include "Platform/windows/Pet/Win32MouseCompanionRendererBackendRegistry.h"
#include "Platform/windows/Pet/Win32MouseCompanionPlaceholderRenderer.h"
#include "MouseFx/Utils/StringUtils.h"

#include <algorithm>
#include <cstdlib>

namespace mousefx::windows {
namespace {

constexpr const char* kAutoBackend = "auto";

void PrioritizePreferredBackend(
    std::vector<Win32MouseCompanionRendererBackendRegistry::Descriptor>* descriptors,
    const std::string& preferredBackendName) {
    if (!descriptors || preferredBackendName.empty() || preferredBackendName == kAutoBackend) {
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
    if (preferredBackendName.empty() || preferredBackendName == kAutoBackend) {
        return "auto_highest_priority_selected";
    }
    if (selectedBackendName == preferredBackendName) {
        return "preferred_backend_selected";
    }
    return preferredRegistered ? "preferred_backend_fallback_selected" : "preferred_backend_not_registered";
}

} // namespace

std::string GetEffectiveWin32MouseCompanionRendererBackendPreference() {
    const char* raw = std::getenv("MFX_WIN32_MOUSE_COMPANION_RENDERER_BACKEND");
    const std::string normalized = ToLowerAscii(TrimAscii(raw ? raw : ""));
    if (normalized.empty()) {
        return kAutoBackend;
    }
    return normalized;
}

Win32MouseCompanionRendererBackendSelection SelectDefaultWin32MouseCompanionRendererBackend(
    const std::string& preferredBackendName) {
    RegisterWin32MouseCompanionPlaceholderRendererBackend();
    Win32MouseCompanionRendererBackendSelection selection{};
    selection.preferredBackendName =
        ToLowerAscii(TrimAscii(preferredBackendName.empty()
                                   ? GetEffectiveWin32MouseCompanionRendererBackendPreference()
                                   : preferredBackendName));

    auto descriptors = Win32MouseCompanionRendererBackendRegistry::Instance().ListByPriority();
    const bool preferredRegistered =
        selection.preferredBackendName.empty() || selection.preferredBackendName == kAutoBackend
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
            selection.failureReason = "backend_create_failed:" + descriptor.name;
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
