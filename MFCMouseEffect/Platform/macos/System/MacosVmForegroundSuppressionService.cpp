#include "pch.h"

#include "Platform/macos/System/MacosVmForegroundSuppressionService.h"

#include "MouseFx/Utils/StringUtils.h"

#include <cstdlib>

namespace mousefx {
namespace {

bool IsFalseLike(const std::string& value) {
    return value == "0" ||
           value == "false" ||
           value == "off" ||
           value == "no" ||
           value == "disable" ||
           value == "disabled";
}

} // namespace

bool MacosVmForegroundSuppressionService::ShouldSuppress(uint64_t nowTickMs) {
    if (!envEnabledResolved_) {
        envEnabledCached_ = IsSuppressionEnabledByEnv();
        envEnabledResolved_ = true;
    }
    if (!envEnabledCached_) {
        return false;
    }

    if ((nowTickMs - lastCheckTickMs_) < kCheckIntervalMs) {
        return lastResult_;
    }

    lastCheckTickMs_ = nowTickMs;
    lastResult_ = IsVmForegroundProcess(foregroundProcessService_.CurrentProcessBaseName());
    return lastResult_;
}

bool MacosVmForegroundSuppressionService::IsSuppressionEnabledByEnv() {
    const char* raw = std::getenv("MFX_VM_FOREGROUND_SUPPRESSION");
    if (!raw) {
        return true;
    }
    std::string value = ToLowerAscii(TrimAscii(raw));
    if (value.empty()) {
        return true;
    }
    return !IsFalseLike(value);
}

bool MacosVmForegroundSuppressionService::IsVmForegroundProcess(const std::string& processBaseName) {
    return ContainsVmToken(processBaseName);
}

bool MacosVmForegroundSuppressionService::ContainsVmToken(const std::string& input) {
    if (input.empty()) {
        return false;
    }
    const std::string lower = ToLowerAscii(input);
    for (const char* token : kVmTokens) {
        if (lower.find(token) != std::string::npos) {
            return true;
        }
    }
    return false;
}

} // namespace mousefx
