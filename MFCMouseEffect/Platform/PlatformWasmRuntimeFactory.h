#pragma once

#include <memory>
#include <string>

namespace mousefx::wasm {
class IWasmRuntime;
}

namespace mousefx::platform {

// Creates and initializes the platform dynamic-bridge WASM runtime.
// Returns nullptr when the runtime cannot be initialized.
std::unique_ptr<mousefx::wasm::IWasmRuntime> CreateDynamicBridgeWasmRuntime(std::string* outError);

// Creates the platform static wasm3-backed WASM runtime.
// Returns nullptr when the runtime backend is not available on this platform/build.
std::unique_ptr<mousefx::wasm::IWasmRuntime> CreateWasm3StaticRuntime(std::string* outError);

} // namespace mousefx::platform
