#pragma once

#include <cstdint>
#include <string>

namespace mousefx {

struct MouseCompanionPluginPhase0Snapshot {
    bool hostReady{false};
    std::string hostPhase;
    std::string activePluginId;
    std::string activePluginVersion;
    std::string engineApiVersion;
    std::string compatibilityStatus;
    std::string fallbackReason;
    std::string lastEventName;
    uint64_t lastEventTickMs{0};
    uint64_t eventCount{0};
};

// Phase-0 plugin host keeps compatibility runtime alive while old pet backend is removed.
// It does not render or solve skeleton motion; it only exposes stable plugin diagnostics.
class MouseCompanionPluginHostPhase0 final {
public:
    MouseCompanionPluginHostPhase0() = default;

    void Reset(bool configEnabled, uint64_t nowTickMs);
    void OnConfigChanged(bool configEnabled, uint64_t nowTickMs);
    void OnInputEvent(const char* eventName, uint64_t nowTickMs);
    MouseCompanionPluginPhase0Snapshot Snapshot() const;

private:
    bool configEnabled_{false};
    uint64_t eventCount_{0};
    std::string lastEventName_;
    uint64_t lastEventTickMs_{0};
};

} // namespace mousefx
