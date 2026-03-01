#pragma once

#include "MouseFx/Core/System/IForegroundSuppressionService.h"
#include "Platform/macos/System/MacosForegroundProcessService.h"

#include <array>
#include <cstdint>
#include <string>

namespace mousefx {

class MacosVmForegroundSuppressionService final : public IForegroundSuppressionService {
public:
    bool ShouldSuppress(uint64_t nowTickMs) override;

private:
    static bool IsSuppressionEnabledByEnv();
    static bool IsVmForegroundProcess(const std::string& processBaseName);
    static bool ContainsVmToken(const std::string& input);

    static constexpr std::array<const char*, 11> kVmTokens = {
        "vmware",
        "virtualbox",
        "virtual box",
        "virtualboxvm",
        "vmconnect",
        "qemu",
        "virt-viewer",
        "remote-viewer",
        "parallels",
        "prl_vm",
        "hyper-v",
    };
    static constexpr uint64_t kCheckIntervalMs = 800;

    MacosForegroundProcessService foregroundProcessService_{};
    uint64_t lastCheckTickMs_ = 0;
    bool lastResult_ = false;
    bool envEnabledCached_ = false;
    bool envEnabledResolved_ = false;
};

} // namespace mousefx
