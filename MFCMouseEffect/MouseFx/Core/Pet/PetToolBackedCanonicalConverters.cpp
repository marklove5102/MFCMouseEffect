#include "pch.h"
#include "MouseFx/Core/Pet/PetInterfaces.h"

#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#if !defined(_WIN32)
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace mousefx::pet {
namespace {

std::string ShellQuoteSingle(const std::string& value) {
    std::string quoted;
    quoted.reserve(value.size() + 2);
    quoted.push_back('\'');
    for (const char ch : value) {
        if (ch == '\'') {
            quoted += "'\"'\"'";
        } else {
            quoted.push_back(ch);
        }
    }
    quoted.push_back('\'');
    return quoted;
}

bool ReplaceAll(std::string* text, const std::string& needle, const std::string& replacement) {
    if (!text || needle.empty()) {
        return false;
    }
    bool replaced = false;
    size_t pos = 0;
    while ((pos = text->find(needle, pos)) != std::string::npos) {
        text->replace(pos, needle.size(), replacement);
        pos += replacement.size();
        replaced = true;
    }
    return replaced;
}

std::string BuildCommand(std::string commandTemplate,
                         const std::filesystem::path& sourcePath,
                         const std::filesystem::path& targetPath) {
    const std::string sourceQuoted = ShellQuoteSingle(sourcePath.string());
    const std::string targetQuoted = ShellQuoteSingle(targetPath.string());
    const bool replacedSrc = ReplaceAll(&commandTemplate, "{src}", sourceQuoted);
    const bool replacedDst = ReplaceAll(&commandTemplate, "{dst}", targetQuoted);
    if (!replacedSrc && !replacedDst) {
        commandTemplate += " ";
        commandTemplate += sourceQuoted;
        commandTemplate += " ";
        commandTemplate += targetQuoted;
    }
    return commandTemplate;
}

int NormalizeExitCode(int rawExitCode) {
    if (rawExitCode < 0) {
        return rawExitCode;
    }
#if defined(_WIN32)
    return rawExitCode;
#else
    if (WIFEXITED(rawExitCode)) {
        return WEXITSTATUS(rawExitCode);
    }
    return rawExitCode;
#endif
}

std::optional<int> ExecuteCommand(const std::string& command) {
    const int raw = std::system(command.c_str());
    if (raw < 0) {
        return std::nullopt;
    }
    return NormalizeExitCode(raw);
}

bool IsCanonicalCacheUsable(const std::filesystem::path& canonicalPath, const std::filesystem::path& sourcePath) {
    std::error_code ec;
    if (!std::filesystem::exists(canonicalPath, ec) || ec) {
        return false;
    }
    const auto sourceTime = std::filesystem::last_write_time(sourcePath, ec);
    if (ec) {
        return false;
    }
    const auto targetTime = std::filesystem::last_write_time(canonicalPath, ec);
    if (ec) {
        return false;
    }
    return targetTime >= sourceTime;
}

std::string Trim(std::string value) {
    size_t start = 0;
    while (start < value.size() &&
           std::isspace(static_cast<unsigned char>(value[start])) != 0) {
        ++start;
    }
    if (start >= value.size()) {
        return {};
    }

    size_t end = value.size();
    while (end > start &&
           std::isspace(static_cast<unsigned char>(value[end - 1])) != 0) {
        --end;
    }
    return value.substr(start, end - start);
}

void AppendCommandPart(const std::string& part, std::vector<std::string>* out) {
    if (!out) {
        return;
    }
    const std::string trimmed = Trim(part);
    if (!trimmed.empty()) {
        out->push_back(trimmed);
    }
}

std::vector<std::string> ParseCommandTemplateList(const std::string& value) {
    std::vector<std::string> commands{};
    std::string normalized = value;
    for (char& ch : normalized) {
        if (ch == '\r') {
            ch = '\n';
        }
    }

    std::istringstream lines(normalized);
    std::string line;
    while (std::getline(lines, line)) {
        size_t cursor = 0;
        while (true) {
            const size_t sep = line.find("||", cursor);
            if (sep == std::string::npos) {
                AppendCommandPart(line.substr(cursor), &commands);
                break;
            }
            AppendCommandPart(line.substr(cursor, sep - cursor), &commands);
            cursor = sep + 2;
        }
    }
    return commands;
}

std::vector<std::string> ResolveCommandTemplates(const char* envVarName,
                                                 const std::vector<std::string>& defaults) {
    if (envVarName && envVarName[0] != '\0') {
        if (const char* value = std::getenv(envVarName); value && value[0] != '\0') {
            const std::vector<std::string> fromEnv = ParseCommandTemplateList(value);
            if (!fromEnv.empty()) {
                return fromEnv;
            }
            return {};
        }
    }
    return defaults;
}

std::string ExtractExecutableToken(const std::string& commandTemplate) {
    const std::string command = Trim(commandTemplate);
    if (command.empty()) {
        return {};
    }

    size_t cursor = 0;
    const char first = command[cursor];
    if (first == '"' || first == '\'') {
        const char quote = first;
        const size_t end = command.find(quote, cursor + 1);
        if (end == std::string::npos) {
            return {};
        }
        return command.substr(cursor + 1, end - (cursor + 1));
    }

    while (cursor < command.size() &&
           std::isspace(static_cast<unsigned char>(command[cursor])) == 0) {
        ++cursor;
    }
    return command.substr(0, cursor);
}

bool IsCommandAvailable(const std::string& executableToken) {
    if (executableToken.empty()) {
        return false;
    }

    const std::filesystem::path executablePath(executableToken);
    const bool explicitPath =
        executableToken.find('/') != std::string::npos ||
        executableToken.find('\\') != std::string::npos;

#if defined(_WIN32)
    std::error_code ec;
    if (explicitPath) {
        return std::filesystem::exists(executablePath, ec) && !ec;
    }
    return true;
#else
    if (explicitPath) {
        return ::access(executablePath.string().c_str(), X_OK) == 0;
    }

    const char* pathEnv = std::getenv("PATH");
    if (!pathEnv || pathEnv[0] == '\0') {
        return false;
    }
    std::istringstream stream(pathEnv);
    std::string dir;
    while (std::getline(stream, dir, ':')) {
        const std::filesystem::path base = dir.empty() ? std::filesystem::path(".") : std::filesystem::path(dir);
        const std::filesystem::path candidate = base / executablePath;
        if (::access(candidate.string().c_str(), X_OK) == 0) {
            return true;
        }
    }
    return false;
#endif
}

class ToolBackedCanonicalFormatConverter final : public IModelFormatConverter {
public:
    ToolBackedCanonicalFormatConverter(ModelFormat format,
                                       std::string diagnosticPrefix,
                                       std::string envVarName,
                                       std::vector<std::string> defaultCommandTemplates)
        : format_(format),
          diagnosticPrefix_(std::move(diagnosticPrefix)),
          envVarName_(std::move(envVarName)),
          defaultCommandTemplates_(std::move(defaultCommandTemplates)) {}

    bool Supports(ModelFormat sourceFormat) const override {
        return sourceFormat == format_;
    }

    bool ConvertToCanonicalGlb(const std::string& sourcePath,
                               ModelFormat sourceFormat,
                               ModelConversionResult* outResult) override {
        if (!outResult) {
            return false;
        }
        *outResult = {};
        if (!Supports(sourceFormat)) {
            return false;
        }
        if (sourcePath.empty()) {
            outResult->warnings.push_back(diagnosticPrefix_ + ".empty_source_path");
            return false;
        }

        const std::filesystem::path source = std::filesystem::path(sourcePath).lexically_normal();
        if (!std::filesystem::exists(source)) {
            outResult->warnings.push_back(diagnosticPrefix_ + ".source_missing");
            return false;
        }

        const std::filesystem::path sourceDir = source.parent_path();
        const std::filesystem::path canonicalDir = sourceDir / "canonical";
        const std::filesystem::path canonicalPath = canonicalDir / (source.stem().string() + ".glb");

        std::error_code ec;
        std::filesystem::create_directories(canonicalDir, ec);
        if (ec) {
            outResult->warnings.push_back(diagnosticPrefix_ + ".create_canonical_dir_failed: " + ec.message());
            return false;
        }

        if (IsCanonicalCacheUsable(canonicalPath, source)) {
            outResult->canonicalGlbPath = canonicalPath.string();
            outResult->converted = true;
            outResult->warnings.push_back(diagnosticPrefix_ + ".reuse_cached_canonical: " + canonicalPath.filename().string());
            return true;
        }

        const std::vector<std::string> commands = ResolveCommandTemplates(
            envVarName_.empty() ? nullptr : envVarName_.c_str(),
            defaultCommandTemplates_);
        if (commands.empty()) {
            outResult->warnings.push_back(diagnosticPrefix_ + ".backend_unavailable");
            return false;
        }

        bool hadAvailableBackend = false;
        for (size_t i = 0; i < commands.size(); ++i) {
            const std::string executable = ExtractExecutableToken(commands[i]);
            if (executable.empty()) {
                outResult->warnings.push_back(
                    diagnosticPrefix_ + ".command_template_invalid[" + std::to_string(i) + "]");
                continue;
            }
            if (!IsCommandAvailable(executable)) {
                outResult->warnings.push_back(
                    diagnosticPrefix_ + ".backend_unavailable[" + std::to_string(i) + "]: " + executable);
                continue;
            }

            hadAvailableBackend = true;
            const std::string command = BuildCommand(commands[i], source, canonicalPath);
            const std::optional<int> exitCode = ExecuteCommand(command);
            if (!exitCode.has_value()) {
                outResult->warnings.push_back(
                    diagnosticPrefix_ + ".command_exec_failed[" + std::to_string(i) + "]");
                continue;
            }
            if (exitCode.value() != 0) {
                outResult->warnings.push_back(
                    diagnosticPrefix_ + ".command_failed[" + std::to_string(i) + "]: exit=" + std::to_string(exitCode.value()));
                continue;
            }

            if (!std::filesystem::exists(canonicalPath)) {
                outResult->warnings.push_back(diagnosticPrefix_ + ".output_missing");
                continue;
            }

            outResult->canonicalGlbPath = canonicalPath.string();
            outResult->converted = true;
            outResult->warnings.push_back(diagnosticPrefix_ + ".exported_canonical: " + canonicalPath.filename().string());
            return true;
        }

        if (!hadAvailableBackend) {
            outResult->warnings.push_back(diagnosticPrefix_ + ".backend_unavailable");
        }
        return false;
    }

private:
    ModelFormat format_{ModelFormat::Unknown};
    std::string diagnosticPrefix_{};
    std::string envVarName_{};
    std::vector<std::string> defaultCommandTemplates_{};
};

} // namespace

std::unique_ptr<IModelFormatConverter> CreateToolBackedUsdzCanonicalFormatConverter() {
    std::vector<std::string> defaults{};
#if defined(__APPLE__)
    defaults.emplace_back("xcrun usdz_converter {src} {dst}");
    defaults.emplace_back("usdz_converter {src} {dst}");
#endif
    return std::make_unique<ToolBackedCanonicalFormatConverter>(
        ModelFormat::Usdz,
        "converter.usdz",
        "MFX_PET_USDZ_TO_GLB_COMMAND",
        std::move(defaults));
}

std::unique_ptr<IModelFormatConverter> CreateToolBackedFbxCanonicalFormatConverter() {
    std::vector<std::string> defaults{};
    defaults.emplace_back("FBX2glTF --binary --input {src} --output {dst}");
    defaults.emplace_back("fbx2gltf --binary --input {src} --output {dst}");
    return std::make_unique<ToolBackedCanonicalFormatConverter>(
        ModelFormat::Fbx,
        "converter.fbx",
        "MFX_PET_FBX_TO_GLB_COMMAND",
        std::move(defaults));
}

} // namespace mousefx::pet
