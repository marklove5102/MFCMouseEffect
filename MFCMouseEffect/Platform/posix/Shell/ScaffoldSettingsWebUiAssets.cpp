#include "pch.h"

#include "Platform/posix/Shell/ScaffoldSettingsWebUiAssets.h"
#include "Platform/posix/Shell/ScaffoldSettingsWebUiAssets.Internal.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string_view>
#include <utility>

namespace mousefx::platform::scaffold {
namespace {

bool PathExists(const std::filesystem::path& path) {
    std::error_code ec;
    return std::filesystem::exists(path, ec) && !ec;
}

bool IsDirectory(const std::filesystem::path& path) {
    std::error_code ec;
    return std::filesystem::is_directory(path, ec) && !ec;
}

bool HasRequiredAsset(const std::filesystem::path& baseDir, std::string_view fileName) {
    if (baseDir.empty()) {
        return false;
    }
    return PathExists(baseDir / std::filesystem::path(fileName));
}

bool HasPreferredWebUiAssets(const std::filesystem::path& baseDir) {
    return HasRequiredAsset(baseDir, "index.html") &&
        HasRequiredAsset(baseDir, "app.js") &&
        HasRequiredAsset(baseDir, "app-core.js") &&
        HasRequiredAsset(baseDir, "app-actions.js") &&
        HasRequiredAsset(baseDir, "app-gesture-debug.js") &&
        HasRequiredAsset(baseDir, "settings-form-input-indicator.js") &&
        HasRequiredAsset(baseDir, "input-indicator-settings.svelte.js");
}

void AddWebUiDirIfExistsUnique(
    const std::filesystem::path& dir,
    std::vector<std::filesystem::path>* outDirs) {
    if (!outDirs || dir.empty()) {
        return;
    }
    if (!IsDirectory(dir)) {
        return;
    }
    for (const auto& existing : *outDirs) {
        if (existing == dir) {
            return;
        }
    }
    outDirs->push_back(dir);
}

bool TryReadWebUiFile(const std::filesystem::path& filePath, std::vector<uint8_t>* outBytes) {
    if (!outBytes) {
        return false;
    }

    std::ifstream input(filePath, std::ios::binary);
    if (!input.is_open()) {
        return false;
    }

    input.seekg(0, std::ios::end);
    const std::streamoff size = input.tellg();
    if (size <= 0 || size > static_cast<std::streamoff>(4 * 1024 * 1024)) {
        return false;
    }

    input.seekg(0, std::ios::beg);
    outBytes->resize(static_cast<size_t>(size));
    input.read(reinterpret_cast<char*>(outBytes->data()), size);
    return input.good();
}

void AddSourceTreeWebUiDir(std::vector<std::filesystem::path>* outDirs) {
    if (!outDirs) {
        return;
    }
    const std::filesystem::path sourcePath(__FILE__);
    if (!sourcePath.is_absolute()) {
        return;
    }

    std::filesystem::path cursor = sourcePath.parent_path();
    while (!cursor.empty()) {
        const std::filesystem::path candidate = cursor / "WebUI";
        if (IsDirectory(candidate) && HasRequiredAsset(candidate, "index.html")) {
            AddWebUiDirIfExistsUnique(candidate, outDirs);
            return;
        }
        if (!cursor.has_parent_path() || cursor.parent_path() == cursor) {
            break;
        }
        cursor = cursor.parent_path();
    }
}

} // namespace

std::vector<std::filesystem::path> BuildWebUiBaseDirs() {
    std::vector<std::filesystem::path> dirs;

    const char* overrideDir = std::getenv("MFX_SCAFFOLD_WEBUI_DIR");
    if (overrideDir && *overrideDir != '\0') {
        AddWebUiDirIfExistsUnique(std::filesystem::path(overrideDir), &dirs);
        return dirs;
    }

    // Keep dev runtime stable: prefer source-tree WebUI over accidental stale cwd copies.
    AddSourceTreeWebUiDir(&dirs);

    std::error_code ec;
    const std::filesystem::path cwd = std::filesystem::current_path(ec);
    if (!ec && !cwd.empty()) {
        AddWebUiDirIfExistsUnique(cwd / "MFCMouseEffect" / "WebUI", &dirs);
        AddWebUiDirIfExistsUnique(cwd / "WebUI", &dirs);
        AddWebUiDirIfExistsUnique(cwd.parent_path() / "MFCMouseEffect" / "WebUI", &dirs);
    }

    std::stable_sort(
        dirs.begin(),
        dirs.end(),
        [](const std::filesystem::path& lhs, const std::filesystem::path& rhs) {
            const bool lhsPreferred = HasPreferredWebUiAssets(lhs);
            const bool rhsPreferred = HasPreferredWebUiAssets(rhs);
            if (lhsPreferred != rhsPreferred) {
                return lhsPreferred && !rhsPreferred;
            }
            return false;
        });
    if (!dirs.empty()) {
        std::vector<std::filesystem::path> deduped;
        deduped.reserve(dirs.size());
        for (const auto& dir : dirs) {
            bool exists = false;
            for (const auto& existing : deduped) {
                if (existing == dir) {
                    exists = true;
                    break;
                }
            }
            if (!exists) {
                deduped.push_back(dir);
            }
        }
        dirs.swap(deduped);
    }
    return dirs;
}

bool TryLoadWebUiAsset(
    const std::vector<std::filesystem::path>& baseDirs,
    const std::string& requestPath,
    WebUiAsset* outAsset) {
    if (!outAsset || baseDirs.empty()) {
        return false;
    }

    const std::string path = NormalizeWebAssetRequestPath(requestPath);
    if (path.empty()) {
        return false;
    }

    for (const auto& baseDir : baseDirs) {
        const std::filesystem::path diskPath = baseDir / path.substr(1);
        WebUiAsset candidate;
        if (!TryReadWebUiFile(diskPath, &candidate.bytes)) {
            continue;
        }
        candidate.contentType = ContentTypeForWebPath(path);
        *outAsset = std::move(candidate);
        return true;
    }
    return false;
}

std::string BuildMissingWebUiMessage() {
    return "Scaffold WebUI assets not found. Build WebUIWorkspace to populate WebUI/*.svelte.js.";
}

} // namespace mousefx::platform::scaffold
