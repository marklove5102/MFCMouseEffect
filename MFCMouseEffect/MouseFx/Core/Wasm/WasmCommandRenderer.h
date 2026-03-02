#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

#include "MouseFx/Core/Config/EffectConfig.h"
#include "WasmCommandExecutionResult.h"

namespace mousefx::wasm {

class IWasmCommandRenderer {
public:
    virtual ~IWasmCommandRenderer() = default;

    virtual bool SupportsRendering() const = 0;

    virtual CommandExecutionResult Execute(
        const uint8_t* commandBuffer,
        size_t commandBytes,
        const EffectConfig& config,
        const std::wstring& activeManifestPath) = 0;
};

std::unique_ptr<IWasmCommandRenderer> CreatePlatformWasmCommandRenderer();

} // namespace mousefx::wasm
