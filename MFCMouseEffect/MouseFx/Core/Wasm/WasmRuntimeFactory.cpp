#include "pch.h"

#include "WasmRuntimeFactory.h"

#include "NullWasmRuntime.h"
#include "Platform/PlatformTarget.h"
#include "Platform/PlatformWasmRuntimeFactory.h"

namespace mousefx::wasm {

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

#if MFX_PLATFORM_WINDOWS
    std::string error;
    auto runtime = platform::CreateDynamicBridgeWasmRuntime(&error);
    if (runtime) {
        result.runtime = std::move(runtime);
        result.backend = RuntimeBackend::DynamicBridge;
        return result;
    }

    result.runtime = std::make_unique<NullWasmRuntime>();
    result.backend = RuntimeBackend::Null;
    result.fallbackReason = "dynamic bridge unavailable";
    if (!error.empty()) {
        result.fallbackReason += ": " + error;
    }
    return result;
#else
    std::string error;
    auto runtime = platform::CreateWasm3StaticRuntime(&error);
    if (runtime) {
        result.runtime = std::move(runtime);
        result.backend = RuntimeBackend::Wasm3Static;
        return result;
    }

    result.runtime = std::make_unique<NullWasmRuntime>();
    result.backend = RuntimeBackend::Null;
    result.fallbackReason = "wasm3 static runtime unavailable";
    if (!error.empty()) {
        result.fallbackReason += ": " + error;
    }
    return result;
#endif
}

std::unique_ptr<IWasmRuntime> CreateDefaultRuntime() {
    RuntimeCreationResult result = CreateDefaultRuntimeWithDiagnostics();
    return std::move(result.runtime);
}

} // namespace mousefx::wasm
