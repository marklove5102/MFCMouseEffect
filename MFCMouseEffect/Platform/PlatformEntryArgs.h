#pragma once

#include <string>
#include <vector>

namespace mousefx::platform {

// Canonical process arguments passed to platform entry runner.
struct PlatformEntryArgs {
    std::vector<std::string> argvUtf8{};
};

} // namespace mousefx::platform
