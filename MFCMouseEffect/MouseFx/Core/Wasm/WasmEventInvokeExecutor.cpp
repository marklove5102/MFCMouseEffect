#include "pch.h"

#include "WasmEventInvokeExecutor.h"

#include <memory>
#include <vector>

namespace mousefx::wasm {

namespace {

IWasmCommandRenderer& DefaultCommandRenderer() {
    static std::unique_ptr<IWasmCommandRenderer> renderer = CreatePlatformWasmCommandRenderer();
    return *renderer;
}

} // namespace

EventDispatchExecutionResult InvokeEventAndRender(
    WasmEffectHost& host,
    const EventInvokeInput& input,
    const EffectConfig& config) {
    return InvokeEventAndRender(host, input, config, DefaultCommandRenderer());
}

EventDispatchExecutionResult InvokeEventAndRender(
    WasmEffectHost& host,
    const EventInvokeInput& input,
    const EffectConfig& config,
    IWasmCommandRenderer& renderer) {
    EventDispatchExecutionResult result{};
    result.routeActive = host.Enabled() && host.IsPluginLoaded();
    if (!result.routeActive) {
        return result;
    }

    std::vector<uint8_t> commandBuffer;
    result.invokeOk = host.InvokeEvent(input, &commandBuffer);
    if (result.invokeOk && !commandBuffer.empty()) {
        result.render = renderer.Execute(
            commandBuffer.data(),
            commandBuffer.size(),
            config,
            host.Diagnostics().activeManifestPath);
    }

    host.RecordRenderExecution(
        result.render.renderedAny,
        result.render.executedTextCommands,
        result.render.executedImageCommands,
        result.render.throttledCommands,
        result.render.throttledByCapacityCommands,
        result.render.throttledByIntervalCommands,
        result.render.droppedCommands,
        result.render.lastError);
    return result;
}

} // namespace mousefx::wasm
