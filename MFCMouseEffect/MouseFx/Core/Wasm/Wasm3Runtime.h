#pragma once

#include "WasmRuntime.h"

#include <cstdint>
#include <string>
#include <vector>

struct M3Environment;
struct M3Runtime;
struct M3Function;
struct M3Module;

namespace mousefx::wasm {

class Wasm3Runtime final : public IWasmRuntime {
public:
    Wasm3Runtime();
    ~Wasm3Runtime() override;

    bool LoadModuleFromFile(const std::wstring& modulePath, std::string* outError) override;
    void UnloadModule() override;
    bool IsModuleLoaded() const override;

    bool CallGetApiVersion(uint32_t* outApiVersion, std::string* outError) override;
    bool CallOnInput(
        const uint8_t* inputPtr,
        uint32_t inputLen,
        uint8_t* outputPtr,
        uint32_t outputCap,
        uint32_t* outWrittenBytes,
        std::string* outError) override;
    bool CallOnFrame(
        const uint8_t* inputPtr,
        uint32_t inputLen,
        uint8_t* outputPtr,
        uint32_t outputCap,
        uint32_t* outWrittenBytes,
        std::string* outError) override;

    void ResetPluginState() override;

private:
    bool CallPluginBufferFunction(
        M3Function* function,
        const char* stageTag,
        const uint8_t* inputPtr,
        uint32_t inputLen,
        uint8_t* outputPtr,
        uint32_t outputCap,
        uint32_t* outWrittenBytes,
        std::string* outError);
    bool CreateRuntime(std::string* outError);
    void ReleaseRuntime();
    bool LoadModuleFromCachedBytes(std::string* outError);
    bool LinkHostImports(M3Module* module, std::string* outError);
    bool ResolvePluginExports(std::string* outError);
    bool EnsureScratchMemory(
        uint32_t inputLen,
        uint32_t outputCap,
        uint32_t* outInputOffset,
        uint32_t* outOutputOffset,
        std::string* outError);

private:
    M3Environment* environment_ = nullptr;
    M3Runtime* runtime_ = nullptr;
    M3Function* fnGetApiVersion_ = nullptr;
    M3Function* fnOnInput_ = nullptr;
    M3Function* fnOnFrame_ = nullptr;
    M3Function* fnReset_ = nullptr;
    std::vector<uint8_t> moduleBytes_{};
    bool moduleLoaded_ = false;
};

} // namespace mousefx::wasm
