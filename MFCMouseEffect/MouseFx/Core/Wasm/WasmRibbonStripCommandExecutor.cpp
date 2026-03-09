#include "pch.h"

#include "WasmRibbonStripCommandExecutor.h"

#include "MouseFx/Core/Wasm/WasmPathFillCommandExecutor.h"
#include "MouseFx/Core/Wasm/WasmRibbonStripCommandConfig.h"

#include <utility>

namespace mousefx::wasm {

void ExecuteSpawnRibbonStrip(
    const uint8_t* raw,
    size_t sizeBytes,
    const EffectConfig& config,
    CommandExecutionResult* outResult) {
    if (!outResult) {
        return;
    }

    ResolvedSpawnPathFillCommand resolved{};
    std::string error;
    if (!TryResolveSpawnRibbonStripCommand(raw, sizeBytes, config, &resolved, &error)) {
        outResult->lastError = error.empty() ? "failed to resolve spawn_ribbon_strip command" : error;
        outResult->droppedCommands += 1;
        return;
    }

    ExecuteResolvedPathFill(std::move(resolved), outResult, "failed to render spawn_ribbon_strip command");
}

} // namespace mousefx::wasm
