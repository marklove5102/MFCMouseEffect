#include "pch.h"

#include "Platform/PlatformWasmRuntimeFactory.h"

#include "Platform/PlatformTarget.h"

#if MFX_PLATFORM_WINDOWS
#include "Platform/windows/Wasm/Win32DllWasmRuntime.h"
#else
#include "MouseFx/Core/Wasm/Wasm3Runtime.h"
#endif

namespace mousefx::platform {

std::unique_ptr<mousefx::wasm::IWasmRuntime> CreateDynamicBridgeWasmRuntime(std::string* outError) {
#if MFX_PLATFORM_WINDOWS
    auto runtime = std::make_unique<mousefx::wasm::Win32DllWasmRuntime>();
    if (!runtime->Initialize(outError)) {
        return nullptr;
    }
    return runtime;
#else
    if (outError) {
        *outError = "dynamic bridge runtime is not available on this platform";
    }
    return nullptr;
#endif
}

std::unique_ptr<mousefx::wasm::IWasmRuntime> CreateWasm3StaticRuntime(std::string* outError) {
#if MFX_PLATFORM_WINDOWS
    if (outError) {
        *outError = "wasm3 static runtime is disabled on windows";
    }
    return nullptr;
#else
    if (outError) {
        outError->clear();
    }
    return std::make_unique<mousefx::wasm::Wasm3Runtime>();
#endif
}

} // namespace mousefx::platform
