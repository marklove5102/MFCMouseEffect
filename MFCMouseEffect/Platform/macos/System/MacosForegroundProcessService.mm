#include "pch.h"

#include "Platform/macos/System/MacosForegroundProcessService.h"

#if defined(__APPLE__)
#import <AppKit/AppKit.h>
#endif

#include "MouseFx/Utils/StringUtils.h"

#include <chrono>

namespace mousefx {
namespace {

uint64_t CurrentSteadyTickMs() {
    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now).count());
}

std::string NormalizeProcessNameFromNSString(NSString* name) {
#if !defined(__APPLE__)
    (void)name;
    return {};
#else
    if (name == nil || [name length] == 0) {
        return {};
    }
    const char* raw = [name UTF8String];
    if (!raw || raw[0] == '\0') {
        return {};
    }
    return ToLowerAscii(TrimAscii(std::string(raw)));
#endif
}

std::string ResolveProcessNameFromApp(NSRunningApplication* app) {
#if !defined(__APPLE__)
    (void)app;
    return {};
#else
    if (app == nil) {
        return {};
    }

    NSURL* executableUrl = [app executableURL];
    if (executableUrl != nil) {
        NSString* baseName = [[executableUrl path] lastPathComponent];
        const std::string normalized = NormalizeProcessNameFromNSString(baseName);
        if (!normalized.empty()) {
            return normalized;
        }
    }

    return NormalizeProcessNameFromNSString([app localizedName]);
#endif
}

} // namespace

std::string MacosForegroundProcessService::CurrentProcessBaseName() {
    std::lock_guard<std::mutex> lock(mutex_);
    const uint64_t nowTickMs = CurrentSteadyTickMs();
    if ((nowTickMs - lastCheckTickMs_) < kCacheIntervalMs) {
        return lastProcessBaseName_;
    }

    lastCheckTickMs_ = nowTickMs;
    lastProcessBaseName_ = QueryForegroundProcessBaseName();
    return lastProcessBaseName_;
}

std::string MacosForegroundProcessService::QueryForegroundProcessBaseName() {
#if !defined(__APPLE__)
    return {};
#else
    @autoreleasepool {
        std::string processBaseName =
            ResolveProcessNameFromApp([[NSWorkspace sharedWorkspace] frontmostApplication]);
        if (!processBaseName.empty()) {
            return processBaseName;
        }

        processBaseName = ResolveProcessNameFromApp([NSRunningApplication currentApplication]);
        if (!processBaseName.empty()) {
            return processBaseName;
        }

        processBaseName = NormalizeProcessNameFromNSString([[NSProcessInfo processInfo] processName]);
        if (!processBaseName.empty()) {
            return processBaseName;
        }

        return "unknown";
    }
#endif
}

} // namespace mousefx
