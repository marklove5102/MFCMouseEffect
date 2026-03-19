#include "pch.h"

#include "MouseFx/Core/Control/MouseCompanionPluginHostPhase0.h"

#include "MouseFx/Utils/StringUtils.h"

namespace mousefx {
namespace {

constexpr const char* kHostPhase = "phase0";
constexpr const char* kActivePluginId = "mousefx.pet.native.phase0.noop";
constexpr const char* kActivePluginVersion = "0.1.0";
constexpr const char* kEngineApiVersion = "pet-plugin-v1-draft";
constexpr const char* kCompatibilityPending = "pending_backend_rewrite";
constexpr const char* kCompatibilityDisabled = "disabled_by_config";
constexpr const char* kFallbackBackendRemoved = "backend_removed_pending_rewrite";
constexpr const char* kFallbackConfigDisabled = "config_disabled";

} // namespace

void MouseCompanionPluginHostPhase0::Reset(bool configEnabled, uint64_t nowTickMs) {
    configEnabled_ = configEnabled;
    eventCount_ = 0;
    lastEventName_.clear();
    lastEventTickMs_ = nowTickMs;
}

void MouseCompanionPluginHostPhase0::OnConfigChanged(bool configEnabled, uint64_t nowTickMs) {
    if (configEnabled_ == configEnabled) {
        return;
    }
    Reset(configEnabled, nowTickMs);
}

void MouseCompanionPluginHostPhase0::OnInputEvent(const char* eventName, uint64_t nowTickMs) {
    if (!configEnabled_) {
        return;
    }

    const std::string normalized = ToLowerAscii(TrimAscii(eventName ? eventName : ""));
    if (normalized.empty()) {
        return;
    }

    lastEventName_ = normalized;
    lastEventTickMs_ = nowTickMs;
    ++eventCount_;
}

MouseCompanionPluginPhase0Snapshot MouseCompanionPluginHostPhase0::Snapshot() const {
    MouseCompanionPluginPhase0Snapshot out{};
    out.hostReady = true;
    out.hostPhase = kHostPhase;
    out.activePluginId = kActivePluginId;
    out.activePluginVersion = kActivePluginVersion;
    out.engineApiVersion = kEngineApiVersion;
    out.compatibilityStatus = configEnabled_ ? kCompatibilityPending : kCompatibilityDisabled;
    out.fallbackReason = configEnabled_ ? kFallbackBackendRemoved : kFallbackConfigDisabled;
    out.lastEventName = lastEventName_;
    out.lastEventTickMs = lastEventTickMs_;
    out.eventCount = eventCount_;
    return out;
}

} // namespace mousefx
