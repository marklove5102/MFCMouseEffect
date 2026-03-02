#include "pch.h"

#include "WasmRuntimeFactory.h"

#include "NullWasmRuntime.h"
#include "Platform/PlatformTarget.h"
#include "Platform/PlatformWasmRuntimeFactory.h"

namespace mousefx::wasm {

namespace {

std::string BuildUnavailableReason(const char* prefix, const std::string& error) {
    std::string reason = prefix ? prefix : "";
    if (!error.empty()) {
        reason += ": ";
        reason += error;
    }
    return reason;
}

} // namespace

const char* RuntimeBackendToString(RuntimeBackend backend) {
    switch (backend) {
    case RuntimeBackend::DynamicBridge:
        return "dynamic_bridge";
    case RuntimeBackend::Wasm3Static:
        return "wasm3_static";
    case RuntimeBackend::Null:
    default:
        return "null";
    }
}

std::unique_ptr<IWasmRuntime> CreateRuntime(RuntimeBackend backend) {
    switch (backend) {
    case RuntimeBackend::DynamicBridge: {
        std::string error;
        auto runtime = platform::CreateDynamicBridgeWasmRuntime(&error);
        if (runtime) {
            return runtime;
        }
        return std::make_unique<NullWasmRuntime>();
    }
    case RuntimeBackend::Wasm3Static: {
        std::string error;
        auto runtime = platform::CreateWasm3StaticRuntime(&error);
        if (runtime) {
            return runtime;
        }
        return std::make_unique<NullWasmRuntime>();
    }
    case RuntimeBackend::Null:
    default:
        return std::make_unique<NullWasmRuntime>();
    }
}

RuntimeCreationResult CreateDefaultRuntimeWithDiagnostics() {
    RuntimeCreationResult result{};

    std::string wasm3Error;
    auto wasm3Runtime = platform::CreateWasm3StaticRuntime(&wasm3Error);
    if (wasm3Runtime) {
        result.runtime = std::move(wasm3Runtime);
        result.backend = RuntimeBackend::Wasm3Static;
        return result;
    }

    std::string dynamicError;
    auto dynamicRuntime = platform::CreateDynamicBridgeWasmRuntime(&dynamicError);
    if (dynamicRuntime) {
        result.runtime = std::move(dynamicRuntime);
        result.backend = RuntimeBackend::DynamicBridge;
        return result;
    }

    result.runtime = std::make_unique<NullWasmRuntime>();
    result.backend = RuntimeBackend::Null;
    result.fallbackReason =
        BuildUnavailableReason("wasm3 static runtime unavailable", wasm3Error);
    const std::string dynamicUnavailableReason =
        BuildUnavailableReason("dynamic bridge unavailable", dynamicError);
    if (!dynamicUnavailableReason.empty()) {
        if (!result.fallbackReason.empty()) {
            result.fallbackReason += "; ";
        }
        result.fallbackReason += dynamicUnavailableReason;
    }
    if (result.fallbackReason.empty()) {
        result.fallbackReason = "no runtime backend is available";
    }
    return result;
}

std::unique_ptr<IWasmRuntime> CreateDefaultRuntime() {
    RuntimeCreationResult result = CreateDefaultRuntimeWithDiagnostics();
    return std::move(result.runtime);
}

} // namespace mousefx::wasm
