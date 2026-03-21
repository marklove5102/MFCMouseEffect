#pragma once

#include <memory>
#include <string>
#include <vector>

namespace mousefx::windows {

class IWin32MouseCompanionRendererBackend;

struct Win32MouseCompanionRendererBackendSelection {
    std::unique_ptr<IWin32MouseCompanionRendererBackend> backend{};
    std::string preferredBackendName;
    std::string selectedBackendName;
    std::string selectionReason;
    std::string failureReason;
    std::vector<std::string> availableBackendNames;
};

std::string GetEffectiveWin32MouseCompanionRendererBackendPreference();
Win32MouseCompanionRendererBackendSelection SelectDefaultWin32MouseCompanionRendererBackend(
    const std::string& preferredBackendName = {});
std::unique_ptr<IWin32MouseCompanionRendererBackend> CreateDefaultWin32MouseCompanionRendererBackend();

} // namespace mousefx::windows
