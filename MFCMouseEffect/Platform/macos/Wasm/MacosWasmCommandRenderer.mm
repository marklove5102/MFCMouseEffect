#include "pch.h"

#include "Platform/macos/Wasm/MacosWasmCommandRenderer.h"

#include "Platform/macos/Wasm/MacosWasmTransientOverlay.h"
#include "MouseFx/Core/Wasm/WasmCommandBufferParser.h"
#include "MouseFx/Core/Wasm/WasmPluginImageAssetCatalog.h"
#include "MouseFx/Core/Wasm/WasmPluginAbi.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>

namespace mousefx::platform::macos {

namespace {

constexpr uint32_t kParseCommandLimit = 4096u;
constexpr uint32_t kFallbackWhiteArgb = 0xFFFFFFFFu;

bool HasVisibleAlpha(uint32_t argb) {
    return ((argb >> 24) & 0xFFu) != 0u;
}

std::wstring ResolveImageAssetPath(
    const std::wstring& activeManifestPath,
    uint32_t imageId) {
    std::wstring imagePath;
    std::string ignoredError;
    if (activeManifestPath.empty()) {
        return {};
    }
    if (!mousefx::wasm::WasmPluginImageAssetCatalog::ResolveImageAssetPath(
            activeManifestPath,
            imageId,
            &imagePath,
            &ignoredError)) {
        return {};
    }
    return imagePath;
}

std::wstring ResolveTextById(const mousefx::EffectConfig& config, uint32_t textId) {
    if (!config.textClick.texts.empty()) {
        const size_t idx = static_cast<size_t>(textId % static_cast<uint32_t>(config.textClick.texts.size()));
        return config.textClick.texts[idx];
    }
    static const std::wstring kFallbackTexts[] = {L"WASM", L"MouseFx", L"Click"};
    const size_t idx = static_cast<size_t>(textId % static_cast<uint32_t>(std::size(kFallbackTexts)));
    return kFallbackTexts[idx];
}

uint32_t ResolveTextColorArgb(const mousefx::EffectConfig& config, uint32_t textId, uint32_t commandColorArgb) {
    if (HasVisibleAlpha(commandColorArgb)) {
        return commandColorArgb;
    }
    if (!config.textClick.colors.empty()) {
        const size_t idx = static_cast<size_t>(textId % static_cast<uint32_t>(config.textClick.colors.size()));
        return config.textClick.colors[idx].value;
    }
    return kFallbackWhiteArgb;
}

uint32_t ResolveImageTintArgb(const mousefx::EffectConfig& config, uint32_t commandTintArgb) {
    if (HasVisibleAlpha(commandTintArgb)) {
        return commandTintArgb;
    }
    return config.icon.fillColor.value;
}

class MacosWasmCommandRenderer final : public mousefx::wasm::IWasmCommandRenderer {
public:
    ~MacosWasmCommandRenderer() override {
        CloseAllWasmOverlays();
    }

    bool SupportsRendering() const override {
        return true;
    }

    mousefx::wasm::CommandExecutionResult Execute(
        const uint8_t* commandBuffer,
        size_t commandBytes,
        const mousefx::EffectConfig& config,
        const std::wstring& activeManifestPath) override {
        mousefx::wasm::CommandExecutionResult result{};
        if (!commandBuffer || commandBytes == 0) {
            return result;
        }

        const mousefx::wasm::CommandParseResult parsed = mousefx::wasm::WasmCommandBufferParser::Parse(
            commandBuffer,
            commandBytes,
            kParseCommandLimit);
        result.parsedCommands = static_cast<uint32_t>(parsed.commands.size());
        if (parsed.error != mousefx::wasm::CommandParseError::None) {
            result.lastError = std::string("command parse failed: ") +
                mousefx::wasm::CommandParseErrorToString(parsed.error);
            result.droppedCommands = result.parsedCommands;
            return result;
        }

        uint32_t throttledTextCommands = 0;
        uint32_t throttledImageCommands = 0;
        uint32_t throttledByCapacityCommands = 0;
        uint32_t throttledByIntervalCommands = 0;
        const auto accountThrottle = [&](WasmOverlayRenderResult renderResult, bool isText) -> bool {
            if (renderResult == WasmOverlayRenderResult::ThrottledByCapacity) {
                throttledByCapacityCommands += 1;
            } else if (renderResult == WasmOverlayRenderResult::ThrottledByInterval) {
                throttledByIntervalCommands += 1;
            } else {
                return false;
            }
            if (isText) {
                throttledTextCommands += 1;
            } else {
                throttledImageCommands += 1;
            }
            result.droppedCommands += 1;
            return true;
        };
        for (const auto& record : parsed.commands) {
            if (record.offsetBytes + record.sizeBytes > commandBytes) {
                result.droppedCommands += 1;
                continue;
            }

            const uint8_t* raw = commandBuffer + record.offsetBytes;
            switch (record.kind) {
            case mousefx::wasm::CommandKind::SpawnText: {
                mousefx::wasm::SpawnTextCommandV1 cmd{};
                std::memcpy(&cmd, raw, sizeof(cmd));
                const ScreenPoint pt{
                    static_cast<int32_t>(std::lround(cmd.x)),
                    static_cast<int32_t>(std::lround(cmd.y)),
                };
                const std::wstring text = ResolveTextById(config, cmd.textId);
                const uint32_t color = ResolveTextColorArgb(config, cmd.textId, cmd.colorRgba);
                const WasmOverlayRenderResult renderResult =
                    ShowWasmTextOverlay(pt, text, color, cmd.scale, cmd.lifeMs);
                if (renderResult == WasmOverlayRenderResult::Rendered) {
                    result.executedTextCommands += 1;
                    result.renderedAny = true;
                } else if (accountThrottle(renderResult, true)) {
                } else {
                    result.droppedCommands += 1;
                    result.lastError = "failed to render spawn_text command";
                }
                break;
            }
            case mousefx::wasm::CommandKind::SpawnImage: {
                mousefx::wasm::SpawnImageCommandV1 cmd{};
                std::memcpy(&cmd, raw, sizeof(cmd));
                WasmImageOverlayRequest request{};
                request.screenPt.x = static_cast<int32_t>(std::lround(cmd.x));
                request.screenPt.y = static_cast<int32_t>(std::lround(cmd.y));
                request.assetPath = ResolveImageAssetPath(activeManifestPath, cmd.imageId);
                request.tintArgb = ResolveImageTintArgb(config, cmd.tintRgba);
                request.scale = cmd.scale;
                request.alpha = cmd.alpha;
                request.lifeMs = cmd.lifeMs;
                request.delayMs = cmd.delayMs;
                request.velocityX = cmd.vx;
                request.velocityY = cmd.vy;
                request.accelerationX = cmd.ax;
                request.accelerationY = cmd.ay;
                request.rotationRad = cmd.rotation;
                request.applyTint = HasVisibleAlpha(cmd.tintRgba);
                const WasmOverlayRenderResult renderResult = ShowWasmImageOverlay(request);
                if (renderResult == WasmOverlayRenderResult::Rendered) {
                    result.executedImageCommands += 1;
                    result.renderedAny = true;
                } else if (accountThrottle(renderResult, false)) {
                } else {
                    result.droppedCommands += 1;
                    result.lastError = "failed to render spawn_image command";
                }
                break;
            }
            case mousefx::wasm::CommandKind::SpawnImageAffine: {
                mousefx::wasm::SpawnImageAffineCommandV1 cmd{};
                std::memcpy(&cmd, raw, sizeof(cmd));
                WasmImageOverlayRequest request{};
                request.screenPt.x = static_cast<int32_t>(std::lround(cmd.base.x + cmd.affineDx));
                request.screenPt.y = static_cast<int32_t>(std::lround(cmd.base.y + cmd.affineDy));
                request.assetPath = ResolveImageAssetPath(activeManifestPath, cmd.base.imageId);
                request.tintArgb = ResolveImageTintArgb(config, cmd.base.tintRgba);
                request.scale = cmd.base.scale;
                request.alpha = cmd.base.alpha;
                request.lifeMs = cmd.base.lifeMs;
                request.delayMs = cmd.base.delayMs;
                request.velocityX = cmd.base.vx;
                request.velocityY = cmd.base.vy;
                request.accelerationX = cmd.base.ax;
                request.accelerationY = cmd.base.ay;
                request.rotationRad = cmd.base.rotation;
                request.applyTint = HasVisibleAlpha(cmd.base.tintRgba);
                const WasmOverlayRenderResult renderResult = ShowWasmImageOverlay(request);
                if (renderResult == WasmOverlayRenderResult::Rendered) {
                    result.executedImageCommands += 1;
                    result.renderedAny = true;
                } else if (accountThrottle(renderResult, false)) {
                } else {
                    result.droppedCommands += 1;
                    result.lastError = "failed to render spawn_image_affine command";
                }
                break;
            }
            default:
                result.droppedCommands += 1;
                break;
            }
        }

        result.throttledCommands = throttledTextCommands + throttledImageCommands;
        result.throttledByCapacityCommands = throttledByCapacityCommands;
        result.throttledByIntervalCommands = throttledByIntervalCommands;

        return result;
    }
};

} // namespace

std::unique_ptr<mousefx::wasm::IWasmCommandRenderer> CreateMacosWasmCommandRenderer() {
    return std::make_unique<MacosWasmCommandRenderer>();
}

} // namespace mousefx::platform::macos
