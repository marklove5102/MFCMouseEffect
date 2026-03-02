#pragma once

#include <cstddef>
#include <cstdint>

#include "MouseFx/Core/Config/EffectConfig.h"
#include "WasmCommandExecutionResult.h"

namespace mousefx::wasm {

class WasmClickCommandExecutor final {
public:
    static CommandExecutionResult Execute(
        const uint8_t* commandBuffer,
        size_t commandBytes,
        const EffectConfig& config,
        const std::wstring& activeManifestPath);
};

} // namespace mousefx::wasm
