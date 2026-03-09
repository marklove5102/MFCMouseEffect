#pragma once

#include <cstddef>
#include <cstdint>

#include "MouseFx/Core/Config/EffectConfig.h"
#include "MouseFx/Core/Wasm/WasmCommandExecutionResult.h"
#include "MouseFx/Core/Wasm/WasmPathFillCommandConfig.h"

namespace mousefx::wasm {

void ExecuteSpawnPathFill(
    const uint8_t* raw,
    size_t sizeBytes,
    const EffectConfig& config,
    CommandExecutionResult* outResult);

void ExecuteResolvedPathFill(
    ResolvedSpawnPathFillCommand resolved,
    CommandExecutionResult* outResult,
    const char* failureMessage);

} // namespace mousefx::wasm
