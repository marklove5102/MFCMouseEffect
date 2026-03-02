#pragma once

#include <memory>

#include "MouseFx/Core/Wasm/WasmCommandRenderer.h"

namespace mousefx::platform::macos {

std::unique_ptr<mousefx::wasm::IWasmCommandRenderer> CreateMacosWasmCommandRenderer();

} // namespace mousefx::platform::macos
