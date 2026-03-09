#pragma once

#include <cstddef>
#include <cstdint>

#include "MouseFx/Core/Config/EffectConfig.h"
#include "MouseFx/Core/Wasm/WasmCommandExecutionResult.h"

namespace mousefx::wasm {

void ExecuteSpawnRibbonStrip(
    const uint8_t* raw,
    size_t sizeBytes,
    const EffectConfig& config,
    CommandExecutionResult* outResult);

} // namespace mousefx::wasm
