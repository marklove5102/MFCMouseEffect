#pragma once

#include "MouseFx/Core/Config/EffectConfig.h"

#include <cstdint>
#include <string>

namespace mousefx::platform::macos::wasm_render_resolver {

bool HasVisibleAlpha(uint32_t argb);
std::wstring ResolveImageAssetPath(const std::wstring& activeManifestPath, uint32_t imageId);
std::wstring ResolveTextById(const mousefx::EffectConfig& config, uint32_t textId);
uint32_t ResolveTextColorArgb(const mousefx::EffectConfig& config, uint32_t textId, uint32_t commandColorArgb);
uint32_t ResolveImageTintArgb(const mousefx::EffectConfig& config, uint32_t commandTintArgb);

} // namespace mousefx::platform::macos::wasm_render_resolver
