#pragma once

#include "MouseFx/Core/Config/EffectConfig.h"
#include "MouseFx/Core/Wasm/WasmCommandBufferParser.h"
#include "MouseFx/Core/Wasm/WasmCommandExecutionResult.h"

#include <cstddef>
#include <cstdint>
#include <string>

namespace mousefx::platform::macos::wasm_render_dispatch {

struct ThrottleCounters final {
    uint32_t text = 0;
    uint32_t image = 0;
    uint32_t byCapacity = 0;
    uint32_t byInterval = 0;
};

bool ExecuteParsedCommand(
    const mousefx::wasm::CommandRecord& record,
    const uint8_t* commandBuffer,
    size_t commandBytes,
    const mousefx::EffectConfig& config,
    const std::wstring& activeManifestPath,
    mousefx::wasm::CommandExecutionResult* outResult,
    ThrottleCounters* outThrottleCounters);

void ApplyThrottleCounters(const ThrottleCounters& counters, mousefx::wasm::CommandExecutionResult* outResult);

} // namespace mousefx::platform::macos::wasm_render_dispatch
