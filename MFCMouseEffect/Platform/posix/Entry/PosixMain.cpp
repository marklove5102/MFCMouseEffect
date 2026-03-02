#include "Platform/PlatformEntryRunner.h"

int main(int argc, char** argv) {
    mousefx::platform::PlatformEntryArgs entryArgs{};
    if (argc > 0) {
        entryArgs.argvUtf8.reserve(static_cast<size_t>(argc));
    }
    for (int i = 0; i < argc; ++i) {
        entryArgs.argvUtf8.emplace_back(argv[i] ? argv[i] : "");
    }
    return mousefx::platform::RunPlatformEntry(entryArgs);
}
