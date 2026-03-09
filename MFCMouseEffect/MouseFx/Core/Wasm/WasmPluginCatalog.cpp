#include "pch.h"

#include "WasmPluginCatalog.h"

#include "WasmPluginAbi.h"
#include "WasmPluginPaths.h"

#include "MouseFx/Utils/StringUtils.h"

#include <algorithm>
#include <cwctype>
#include <filesystem>
#include <map>
#include <sstream>
#include <system_error>

namespace mousefx::wasm {

namespace {

bool EqualsIgnoreCaseAscii(const std::wstring& lhs, const std::wstring& rhs) {
    if (lhs.size() != rhs.size()) {
        return false;
    }
    for (size_t i = 0; i < lhs.size(); ++i) {
        const wchar_t a = static_cast<wchar_t>(std::towlower(lhs[i]));
        const wchar_t b = static_cast<wchar_t>(std::towlower(rhs[i]));
        if (a != b) {
            return false;
        }
    }
    return true;
}

std::filesystem::path NormalizePath(const std::filesystem::path& path) {
    std::error_code ec;
    const std::filesystem::path canonical = std::filesystem::weakly_canonical(path, ec);
    if (!ec) {
        return canonical.lexically_normal();
    }
    return path.lexically_normal();
}

std::wstring BuildPathKey(const std::filesystem::path& path) {
    std::wstring key = NormalizePath(path).wstring();
    std::transform(key.begin(), key.end(), key.begin(), [](wchar_t ch) {
        return static_cast<wchar_t>(std::towlower(ch));
    });
    return key;
}

bool HasAncestorManifestBelowRoot(
    const std::filesystem::path& rootPath,
    const std::filesystem::path& manifestPath) {
    const std::wstring rootKey = BuildPathKey(rootPath);
    std::filesystem::path current = NormalizePath(manifestPath.parent_path());
    while (true) {
        const std::filesystem::path parent = current.parent_path();
        if (parent.empty() || parent == current) {
            return false;
        }
        current = NormalizePath(parent);
        if (BuildPathKey(current) == rootKey) {
            return false;
        }

        std::error_code ec;
        const std::filesystem::path ancestorManifest = current / L"plugin.json";
        if (std::filesystem::exists(ancestorManifest, ec) &&
            !ec &&
            std::filesystem::is_regular_file(ancestorManifest, ec) &&
            !ec) {
            return true;
        }
    }
}

std::vector<std::filesystem::path> FindManifestFiles(const std::wstring& root) {
    std::vector<std::filesystem::path> files;
    if (root.empty()) {
        return files;
    }

    std::error_code ec;
    const std::filesystem::path rootPath(root);
    if (!std::filesystem::exists(rootPath, ec) || ec) {
        return files;
    }

    std::filesystem::recursive_directory_iterator it(
        rootPath,
        std::filesystem::directory_options::skip_permission_denied,
        ec);
    const std::filesystem::recursive_directory_iterator end;
    while (!ec && it != end) {
        const auto& entry = *it;
        std::error_code localEc;
        if (entry.is_regular_file(localEc) && !localEc) {
            const std::wstring filename = entry.path().filename().wstring();
            if (EqualsIgnoreCaseAscii(filename, L"plugin.json")) {
                if (HasAncestorManifestBelowRoot(rootPath, entry.path())) {
                    it.increment(ec);
                    if (ec) {
                        ec.clear();
                    }
                    continue;
                }
                files.push_back(entry.path());
            }
        }
        it.increment(ec);
        if (ec) {
            ec.clear();
        }
    }
    return files;
}

void AppendCatalogError(
    const std::filesystem::path& manifestPath,
    const std::string& error,
    std::vector<std::string>* errors) {
    if (!errors) {
        return;
    }
    std::ostringstream ss;
    ss << "manifest=" << Utf16ToUtf8(manifestPath.wstring().c_str()) << " error=" << error;
    errors->push_back(ss.str());
}

} // namespace

PluginCatalogResult WasmPluginCatalog::Discover() const {
    return DiscoverFromRoots(WasmPluginPaths::ResolveSearchRoots());
}

PluginCatalogResult WasmPluginCatalog::DiscoverFromRoots(const std::vector<std::wstring>& roots) const {
    PluginCatalogResult result{};
    std::map<std::string, std::filesystem::path> seenPluginIds;

    for (const std::wstring& root : roots) {
        const std::vector<std::filesystem::path> manifests = FindManifestFiles(root);
        for (const std::filesystem::path& manifestPath : manifests) {
            const PluginManifestLoadResult load = WasmPluginManifest::LoadFromFile(manifestPath.wstring());
            if (!load.ok) {
                AppendCatalogError(manifestPath, load.error, &result.errors);
                continue;
            }
            const std::string normalizedId = ToLowerAscii(load.manifest.id);
            if (load.manifest.apiVersion != kPluginApiVersionCurrent) {
                continue;
            }
            const auto duplicateIt = seenPluginIds.find(normalizedId);
            if (duplicateIt != seenPluginIds.end()) {
                continue;
            }

            const std::wstring wasmPath = WasmPluginPaths::ResolveEntryWasmPath(manifestPath.wstring(), load.manifest);
            if (wasmPath.empty()) {
                AppendCatalogError(manifestPath, "Cannot resolve wasm entry path.", &result.errors);
                continue;
            }
            std::error_code ec;
            if (!std::filesystem::exists(std::filesystem::path(wasmPath), ec) || ec) {
                AppendCatalogError(manifestPath, "WASM entry file does not exist.", &result.errors);
                continue;
            }

            DiscoveredPlugin plugin{};
            plugin.manifest = load.manifest;
            plugin.manifestPath = manifestPath.wstring();
            plugin.wasmPath = wasmPath;
            result.plugins.push_back(std::move(plugin));
            seenPluginIds[normalizedId] = manifestPath;
        }
    }

    std::sort(result.plugins.begin(), result.plugins.end(), [](const DiscoveredPlugin& lhs, const DiscoveredPlugin& rhs) {
        return ToLowerAscii(lhs.manifest.id) < ToLowerAscii(rhs.manifest.id);
    });
    return result;
}

} // namespace mousefx::wasm
