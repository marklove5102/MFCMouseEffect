#include "pch.h"

#include "WasmCommandRenderer.h"

#include "WasmCommandBufferParser.h"
#include "Platform/PlatformTarget.h"

#if MFX_PLATFORM_WINDOWS
#include "WasmClickCommandExecutor.h"
#elif MFX_PLATFORM_MACOS
#include "Platform/macos/Wasm/MacosWasmCommandRenderer.h"
#endif

namespace mousefx::wasm {

namespace {

constexpr uint32_t kPosixRenderParseMaxCommands = 4096u;

class PosixDegradedWasmCommandRenderer final : public IWasmCommandRenderer {
public:
    bool SupportsRendering() const override {
        return false;
    }

    CommandExecutionResult Execute(
        const uint8_t* commandBuffer,
        size_t commandBytes,
        const EffectConfig&,
        const std::wstring&) override {
        CommandExecutionResult result{};
        if (!commandBuffer || commandBytes == 0) {
            return result;
        }

        const CommandParseResult parseResult = WasmCommandBufferParser::Parse(
            commandBuffer,
            commandBytes,
            kPosixRenderParseMaxCommands);
        result.parsedCommands = static_cast<uint32_t>(parseResult.commands.size());
        if (parseResult.error != CommandParseError::None) {
            result.lastError = std::string("WASM command buffer parse failed: ") +
                CommandParseErrorToString(parseResult.error);
            result.droppedCommands = result.parsedCommands;
            return result;
        }

        result.droppedCommands = result.parsedCommands;
        if (result.droppedCommands > 0) {
            result.lastError = "WASM command rendering is not supported on this platform yet.";
        }
        return result;
    }
};

#if MFX_PLATFORM_WINDOWS
class WindowsWasmCommandRenderer final : public IWasmCommandRenderer {
public:
    bool SupportsRendering() const override {
        return true;
    }

    CommandExecutionResult Execute(
        const uint8_t* commandBuffer,
        size_t commandBytes,
        const EffectConfig& config,
        const std::wstring& activeManifestPath) override {
        return WasmClickCommandExecutor::Execute(commandBuffer, commandBytes, config, activeManifestPath);
    }
};
#endif

} // namespace

std::unique_ptr<IWasmCommandRenderer> CreatePlatformWasmCommandRenderer() {
#if MFX_PLATFORM_WINDOWS
    return std::make_unique<WindowsWasmCommandRenderer>();
#elif MFX_PLATFORM_MACOS
    return platform::macos::CreateMacosWasmCommandRenderer();
#else
    return std::make_unique<PosixDegradedWasmCommandRenderer>();
#endif
}

} // namespace mousefx::wasm
