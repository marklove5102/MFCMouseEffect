#include "pch.h"

#include "Wasm3Runtime.h"

#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>

#include "WasmRuntimeBridge/third_party/wasm3/source/m3_env.h"
#include "WasmRuntimeBridge/third_party/wasm3/source/wasm3.h"

namespace mousefx::wasm {
namespace {

constexpr uint32_t kRuntimeStackBytes = 256u * 1024u;
constexpr uint32_t kLinearMemoryLimitBytes = 4u * 1024u * 1024u;
constexpr uint32_t kWasmPageBytes = 64u * 1024u;
constexpr uint32_t kScratchAlignment = 16u;

uint32_t AlignUp(uint32_t value, uint32_t alignment) {
    if (alignment == 0) {
        return value;
    }
    const uint32_t remainder = value % alignment;
    return remainder == 0 ? value : (value + alignment - remainder);
}

std::string BuildWasm3Error(const char* prefix, const char* wasm3Result) {
    std::string out;
    if (prefix && prefix[0] != '\0') {
        out.append(prefix);
        out.append(": ");
    }
    out.append(wasm3Result ? wasm3Result : "unknown wasm3 error");
    return out;
}

void SetOutError(std::string* outError, const std::string& value) {
    if (outError) {
        *outError = value;
    }
}

M3Result SuppressLookupFailure(M3Result result) {
    return (result == m3Err_functionLookupFailed) ? m3Err_none : result;
}

m3ApiRawFunction(HostAbortNoArgs) {
    m3ApiTrap(m3Err_trapAbort);
}

m3ApiRawFunction(HostAssemblyScriptAbort) {
    m3ApiGetArg(uint32_t, messageOffset);
    m3ApiGetArg(uint32_t, fileOffset);
    m3ApiGetArg(uint32_t, lineNumber);
    m3ApiGetArg(uint32_t, columnNumber);
    (void)messageOffset;
    (void)fileOffset;
    (void)lineNumber;
    (void)columnNumber;
    m3ApiTrap(m3Err_trapAbort);
}

} // namespace

Wasm3Runtime::Wasm3Runtime() = default;

Wasm3Runtime::~Wasm3Runtime() {
    ReleaseRuntime();
    if (environment_) {
        m3_FreeEnvironment(environment_);
        environment_ = nullptr;
    }
}

bool Wasm3Runtime::LoadModuleFromFile(const std::wstring& modulePath, std::string* outError) {
    if (modulePath.empty()) {
        SetOutError(outError, "module path is empty.");
        return false;
    }

    const std::filesystem::path path(modulePath);
    std::error_code ec;
    if (!std::filesystem::exists(path, ec) || ec) {
        SetOutError(outError, std::string("module file does not exist: ") + path.string());
        return false;
    }
    if (!std::filesystem::is_regular_file(path, ec) || ec) {
        SetOutError(outError, std::string("module path is not a file: ") + path.string());
        return false;
    }

    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if (!input.is_open()) {
        SetOutError(outError, std::string("cannot open module file: ") + path.string());
        return false;
    }

    const std::streamoff fileSize = input.tellg();
    if (fileSize <= 0) {
        SetOutError(outError, "module file is empty.");
        return false;
    }
    if (static_cast<uint64_t>(fileSize) > static_cast<uint64_t>((std::numeric_limits<uint32_t>::max)())) {
        SetOutError(outError, "module file is too large.");
        return false;
    }

    std::vector<uint8_t> wasmBytes(static_cast<size_t>(fileSize), 0);
    input.seekg(0, std::ios::beg);
    input.read(reinterpret_cast<char*>(wasmBytes.data()), fileSize);
    if (!input.good()) {
        SetOutError(outError, "failed to read module bytes.");
        return false;
    }

    ReleaseRuntime();
    if (!CreateRuntime(outError)) {
        return false;
    }
    moduleBytes_ = std::move(wasmBytes);
    if (!LoadModuleFromCachedBytes(outError)) {
        ReleaseRuntime();
        return false;
    }

    SetOutError(outError, "");
    return true;
}

void Wasm3Runtime::UnloadModule() {
    ReleaseRuntime();
    (void)CreateRuntime(nullptr);
}

bool Wasm3Runtime::IsModuleLoaded() const {
    return moduleLoaded_ && runtime_ && fnGetApiVersion_ && fnOnEvent_;
}

bool Wasm3Runtime::CallGetApiVersion(uint32_t* outApiVersion, std::string* outError) {
    if (outApiVersion) {
        *outApiVersion = 0;
    }
    if (!IsModuleLoaded()) {
        SetOutError(outError, "plugin module is not loaded.");
        return false;
    }

    M3Result result = m3_Call(fnGetApiVersion_, 0, nullptr);
    if (result) {
        SetOutError(outError, BuildWasm3Error("m3_Call(get_api_version) failed", result));
        return false;
    }

    uint32_t apiVersion = 0;
    const void* retPointers[1] = { &apiVersion };
    result = m3_GetResults(fnGetApiVersion_, 1, retPointers);
    if (result) {
        SetOutError(outError, BuildWasm3Error("m3_GetResults(get_api_version) failed", result));
        return false;
    }

    if (outApiVersion) {
        *outApiVersion = apiVersion;
    }
    SetOutError(outError, "");
    return true;
}

bool Wasm3Runtime::CallOnEvent(
    const uint8_t* inputPtr,
    uint32_t inputLen,
    uint8_t* outputPtr,
    uint32_t outputCap,
    uint32_t* outWrittenBytes,
    std::string* outError) {
    if (outWrittenBytes) {
        *outWrittenBytes = 0;
    }
    if (!IsModuleLoaded()) {
        SetOutError(outError, "plugin does not export mfx_plugin_on_event.");
        return false;
    }
    if (inputLen > 0 && inputPtr == nullptr) {
        SetOutError(outError, "input pointer is null.");
        return false;
    }
    if (outputCap > 0 && outputPtr == nullptr) {
        SetOutError(outError, "output pointer is null.");
        return false;
    }

    uint32_t inputOffset = 0;
    uint32_t outputOffset = 0;
    if (!EnsureScratchMemory(inputLen, outputCap, &inputOffset, &outputOffset, outError)) {
        return false;
    }

    uint32_t memorySize = 0;
    uint8_t* memory = m3_GetMemory(runtime_, &memorySize, 0);
    if (!memory) {
        SetOutError(outError, "wasm linear memory is unavailable.");
        return false;
    }
    if (inputLen > 0) {
        std::memcpy(memory + inputOffset, inputPtr, inputLen);
    }

    const uint32_t argInputOffset = inputOffset;
    const uint32_t argInputLen = inputLen;
    const uint32_t argOutputOffset = outputOffset;
    const uint32_t argOutputCap = outputCap;
    const void* argPointers[4] = {
        &argInputOffset,
        &argInputLen,
        &argOutputOffset,
        &argOutputCap,
    };

    M3Result result = m3_Call(fnOnEvent_, 4, argPointers);
    if (result) {
        SetOutError(outError, BuildWasm3Error("m3_Call(on_event) failed", result));
        return false;
    }

    uint32_t writtenBytes = 0;
    const void* retPointers[1] = { &writtenBytes };
    result = m3_GetResults(fnOnEvent_, 1, retPointers);
    if (result) {
        SetOutError(outError, BuildWasm3Error("m3_GetResults(on_event) failed", result));
        return false;
    }
    if (writtenBytes > outputCap) {
        SetOutError(outError, "plugin returned bytes larger than output capacity.");
        return false;
    }

    if (writtenBytes > 0) {
        uint32_t memorySizeAfterCall = 0;
        uint8_t* memoryAfterCall = m3_GetMemory(runtime_, &memorySizeAfterCall, 0);
        if (!memoryAfterCall) {
            SetOutError(outError, "wasm linear memory is unavailable after plugin call.");
            return false;
        }
        const uint64_t outputEnd = static_cast<uint64_t>(outputOffset) + static_cast<uint64_t>(writtenBytes);
        if (outputEnd > static_cast<uint64_t>(memorySizeAfterCall)) {
            SetOutError(outError, "plugin output range exceeds wasm linear memory.");
            return false;
        }
        std::memcpy(outputPtr, memoryAfterCall + outputOffset, writtenBytes);
    }

    if (outWrittenBytes) {
        *outWrittenBytes = writtenBytes;
    }
    SetOutError(outError, "");
    return true;
}

void Wasm3Runtime::ResetPluginState() {
    if (!moduleLoaded_ || !runtime_ || !fnReset_) {
        return;
    }
    (void)m3_Call(fnReset_, 0, nullptr);
}

bool Wasm3Runtime::CreateRuntime(std::string* outError) {
    if (!environment_) {
        environment_ = m3_NewEnvironment();
        if (!environment_) {
            SetOutError(outError, "m3_NewEnvironment failed.");
            return false;
        }
    }
    if (runtime_) {
        SetOutError(outError, "");
        return true;
    }

    runtime_ = m3_NewRuntime(environment_, kRuntimeStackBytes, nullptr);
    if (!runtime_) {
        SetOutError(outError, "m3_NewRuntime failed.");
        return false;
    }
    runtime_->memoryLimit = kLinearMemoryLimitBytes;
    SetOutError(outError, "");
    return true;
}

void Wasm3Runtime::ReleaseRuntime() {
    moduleLoaded_ = false;
    fnGetApiVersion_ = nullptr;
    fnOnEvent_ = nullptr;
    fnReset_ = nullptr;
    moduleBytes_.clear();
    if (runtime_) {
        m3_FreeRuntime(runtime_);
        runtime_ = nullptr;
    }
}

bool Wasm3Runtime::LoadModuleFromCachedBytes(std::string* outError) {
    if (!runtime_ || !environment_) {
        SetOutError(outError, "runtime is not initialized.");
        return false;
    }
    if (moduleBytes_.empty()) {
        SetOutError(outError, "module bytes are empty.");
        return false;
    }

    IM3Module module = nullptr;
    M3Result result = m3_ParseModule(
        environment_,
        &module,
        moduleBytes_.data(),
        static_cast<uint32_t>(moduleBytes_.size()));
    if (result) {
        SetOutError(outError, BuildWasm3Error("m3_ParseModule failed", result));
        if (module) {
            m3_FreeModule(module);
        }
        return false;
    }

    result = m3_LoadModule(runtime_, module);
    if (result) {
        SetOutError(outError, BuildWasm3Error("m3_LoadModule failed", result));
        if (module) {
            m3_FreeModule(module);
        }
        return false;
    }

    if (!LinkHostImports(module, outError)) {
        return false;
    }
    if (!ResolvePluginExports(outError)) {
        return false;
    }

    moduleLoaded_ = true;
    SetOutError(outError, "");
    return true;
}

bool Wasm3Runtime::LinkHostImports(M3Module* module, std::string* outError) {
    if (!module) {
        SetOutError(outError, "module is null while linking host imports.");
        return false;
    }

    M3Result result = SuppressLookupFailure(
        m3_LinkRawFunction(module, "env", "abort", "v(iiii)", &HostAssemblyScriptAbort));
    if (result) {
        SetOutError(outError, BuildWasm3Error("m3_LinkRawFunction(env.abort) failed", result));
        return false;
    }

    result = SuppressLookupFailure(
        m3_LinkRawFunction(module, "env", "_abort", "v()", &HostAbortNoArgs));
    if (result) {
        SetOutError(outError, BuildWasm3Error("m3_LinkRawFunction(env._abort) failed", result));
        return false;
    }

    return true;
}

bool Wasm3Runtime::ResolvePluginExports(std::string* outError) {
    if (!runtime_) {
        SetOutError(outError, "runtime is not initialized.");
        return false;
    }

    M3Result result = m3_FindFunction(&fnGetApiVersion_, runtime_, "mfx_plugin_get_api_version");
    if (result) {
        SetOutError(outError, BuildWasm3Error("m3_FindFunction(mfx_plugin_get_api_version) failed", result));
        return false;
    }

    result = m3_FindFunction(&fnOnEvent_, runtime_, "mfx_plugin_on_event");
    if (result) {
        fnOnEvent_ = nullptr;
    }
    if (!fnOnEvent_) {
        SetOutError(outError, "plugin does not export mfx_plugin_on_event.");
        return false;
    }

    result = m3_FindFunction(&fnReset_, runtime_, "mfx_plugin_reset");
    if (result) {
        fnReset_ = nullptr;
    }
    return true;
}

bool Wasm3Runtime::EnsureScratchMemory(
    uint32_t inputLen,
    uint32_t outputCap,
    uint32_t* outInputOffset,
    uint32_t* outOutputOffset,
    std::string* outError) {
    if (!runtime_ || !outInputOffset || !outOutputOffset) {
        SetOutError(outError, "runtime scratch preparation failed.");
        return false;
    }

    const uint32_t inputOffset = 0;
    const uint32_t outputOffset = AlignUp(inputLen, kScratchAlignment);
    const uint64_t requiredBytes = static_cast<uint64_t>(outputOffset) + static_cast<uint64_t>(outputCap);
    if (requiredBytes > static_cast<uint64_t>((std::numeric_limits<uint32_t>::max)())) {
        SetOutError(outError, "scratch memory request exceeds 32-bit addressable range.");
        return false;
    }

    uint32_t currentBytes = m3_GetMemorySize(runtime_);
    if (currentBytes < requiredBytes) {
        const uint32_t requiredPages = static_cast<uint32_t>((requiredBytes + kWasmPageBytes - 1u) / kWasmPageBytes);
        M3Result resizeResult = ResizeMemory(runtime_, requiredPages);
        if (resizeResult) {
            SetOutError(outError, BuildWasm3Error("ResizeMemory failed", resizeResult));
            return false;
        }
        currentBytes = m3_GetMemorySize(runtime_);
    }

    uint32_t memorySize = 0;
    uint8_t* memory = m3_GetMemory(runtime_, &memorySize, 0);
    if (!memory) {
        SetOutError(outError, "wasm memory allocation returned null.");
        return false;
    }
    if (memorySize < requiredBytes || currentBytes < requiredBytes) {
        SetOutError(outError, "wasm memory is smaller than requested scratch range.");
        return false;
    }

    *outInputOffset = inputOffset;
    *outOutputOffset = outputOffset;
    return true;
}

} // namespace mousefx::wasm
