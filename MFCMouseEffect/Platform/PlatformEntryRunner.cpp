#include "pch.h"

#include "Platform/PlatformEntryRunner.h"

#include <memory>

#include "Platform/PlatformAppShellFactory.h"
#include "Platform/PlatformStartupOptionsFactory.h"

namespace mousefx::platform {

namespace {

int RunPlatformEntryWithOptions(const AppShellStartOptions& options) {
    std::unique_ptr<IPlatformAppShell> app = CreatePlatformAppShell();
    if (!app || !app->Initialize(options)) {
        return 0;
    }

    const int code = app->RunMessageLoop();
    app->Shutdown();
    return code;
}

} // namespace

int RunPlatformEntry() {
    return RunPlatformEntryWithOptions(CreatePlatformStartupOptions());
}

int RunPlatformEntry(const PlatformEntryArgs& entryArgs) {
    return RunPlatformEntryWithOptions(CreatePlatformStartupOptions(entryArgs));
}

} // namespace mousefx::platform
