#pragma once

#include <cstdint>
#include <string>

namespace mousefx::wasm {

struct CommandExecutionResult final {
    uint32_t parsedCommands = 0;
    uint32_t executedTextCommands = 0;
    uint32_t executedImageCommands = 0;
    uint32_t throttledCommands = 0;
    uint32_t throttledByCapacityCommands = 0;
    uint32_t throttledByIntervalCommands = 0;
    uint32_t droppedCommands = 0;
    bool renderedAny = false;
    std::string lastError{};
};

} // namespace mousefx::wasm
