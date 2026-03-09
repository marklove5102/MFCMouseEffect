#pragma once

#include "MouseFx/Core/Config/EffectConfig.h"
#include "WasmCommandRenderer.h"
#include "WasmEffectHost.h"

namespace mousefx::wasm {

struct EventDispatchExecutionResult final {
    bool routeActive = false;
    bool invokeOk = false;
    CommandExecutionResult render{};
};

EventDispatchExecutionResult InvokeEventAndRender(
    WasmEffectHost& host,
    const EventInvokeInput& input,
    const EffectConfig& config);

EventDispatchExecutionResult InvokeEventAndRender(
    WasmEffectHost& host,
    const EventInvokeInput& input,
    const EffectConfig& config,
    IWasmCommandRenderer& renderer);

EventDispatchExecutionResult InvokeFrameAndRender(
    WasmEffectHost& host,
    const FrameInvokeInput& input,
    const EffectConfig& config);

EventDispatchExecutionResult InvokeFrameAndRender(
    WasmEffectHost& host,
    const FrameInvokeInput& input,
    const EffectConfig& config,
    IWasmCommandRenderer& renderer);

} // namespace mousefx::wasm
