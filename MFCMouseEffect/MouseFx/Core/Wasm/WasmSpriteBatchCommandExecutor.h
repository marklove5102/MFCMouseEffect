#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include "MouseFx/Core/Config/EffectConfig.h"
#include "MouseFx/Core/Wasm/WasmCommandExecutionResult.h"

namespace mousefx::wasm {

void ExecuteSpawnSpriteBatch(
    const uint8_t* raw,
    size_t sizeBytes,
    const EffectConfig& config,
    const std::wstring& activeManifestPath,
    CommandExecutionResult* outResult);

void ExecuteSpawnQuadBatch(
    const uint8_t* raw,
    size_t sizeBytes,
    const EffectConfig& config,
    const std::wstring& activeManifestPath,
    CommandExecutionResult* outResult);

} // namespace mousefx::wasm
