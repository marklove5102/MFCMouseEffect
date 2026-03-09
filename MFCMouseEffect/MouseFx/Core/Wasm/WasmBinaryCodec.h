#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "WasmPluginAbi.h"

namespace mousefx::wasm {

std::array<uint8_t, sizeof(EventInputV2)> SerializeEventInputV2(const EventInputV2& input);
std::array<uint8_t, sizeof(FrameInputV2)> SerializeFrameInputV2(const FrameInputV2& input);
bool TryReadCommandHeaderV1(
    const uint8_t* buffer,
    size_t bufferBytes,
    size_t offset,
    CommandHeaderV1* outHeader);

} // namespace mousefx::wasm
