#include "pch.h"

#include "WasmPluginManifest.h"

#include "MouseFx/Utils/StringUtils.h"

#include <algorithm>
#include <cwctype>
#include <filesystem>
#include <set>

namespace mousefx::wasm {
namespace {

bool IsAsciiIdChar(char ch) {
    return (ch >= 'a' && ch <= 'z') ||
        (ch >= 'A' && ch <= 'Z') ||
        (ch >= '0' && ch <= '9') ||
        ch == '.' || ch == '_' || ch == '-';
}

bool IsAsciiIdentifier(const std::string& value) {
    if (value.empty()) {
        return false;
    }
    for (char ch : value) {
        if (!IsAsciiIdChar(ch)) {
            return false;
        }
    }
    return true;
}

std::wstring ToLowerWide(std::wstring text) {
    std::transform(text.begin(), text.end(), text.begin(), [](wchar_t ch) -> wchar_t {
        return static_cast<wchar_t>(::towlower(ch));
    });
    return text;
}

bool ContainsParentTraversal(const std::filesystem::path& path) {
    for (const auto& part : path) {
        if (part == L"..") {
            return true;
        }
    }
    return false;
}

bool IsSupportedImageExt(const std::wstring& extLower) {
    static const std::set<std::wstring> kSupported{
        L".png",
        L".jpg",
        L".jpeg",
        L".bmp",
        L".gif",
        L".tif",
        L".tiff",
    };
    return kSupported.find(extLower) != kSupported.end();
}

} // namespace

bool WasmPluginManifest::Validate(const PluginManifest& manifest, std::string* outError) {
    if (!IsAsciiIdentifier(manifest.id)) {
        if (outError) {
            *outError = "Manifest id must use [A-Za-z0-9._-].";
        }
        return false;
    }
    if (TrimAscii(manifest.name).empty()) {
        if (outError) {
            *outError = "Manifest name must not be empty.";
        }
        return false;
    }
    if (TrimAscii(manifest.version).empty()) {
        if (outError) {
            *outError = "Manifest version must not be empty.";
        }
        return false;
    }
    if (manifest.apiVersion == 0) {
        if (outError) {
            *outError = "Manifest api_version must be >= 1.";
        }
        return false;
    }
    if (manifest.entryWasm.empty()) {
        if (outError) {
            *outError = "Manifest entry must not be empty.";
        }
        return false;
    }
    const std::filesystem::path entryPath(manifest.entryWasm);
    if (entryPath.is_absolute()) {
        if (outError) {
            *outError = "Manifest entry must be relative path.";
        }
        return false;
    }
    if (ToLowerWide(entryPath.extension().wstring()) != L".wasm") {
        if (outError) {
            *outError = "Manifest entry must be a .wasm file.";
        }
        return false;
    }
    if (ContainsParentTraversal(entryPath)) {
        if (outError) {
            *outError = "Manifest entry must not use parent traversal.";
        }
        return false;
    }

    for (size_t i = 0; i < manifest.imageAssets.size(); ++i) {
        const std::wstring& asset = manifest.imageAssets[i];
        if (asset.empty()) {
            if (outError) {
                *outError = "Manifest image_assets item must not be empty.";
            }
            return false;
        }
        const std::filesystem::path assetPath(asset);
        if (assetPath.is_absolute()) {
            if (outError) {
                *outError = "Manifest image_assets must use relative path.";
            }
            return false;
        }
        if (ContainsParentTraversal(assetPath)) {
            if (outError) {
                *outError = "Manifest image_assets must not use parent traversal.";
            }
            return false;
        }
        const std::wstring ext = ToLowerWide(assetPath.extension().wstring());
        if (!IsSupportedImageExt(ext)) {
            if (outError) {
                *outError = "Manifest image_assets supports .png/.jpg/.jpeg/.bmp/.gif/.tif/.tiff only.";
            }
            return false;
        }
    }

    if ((manifest.inputKindsMask & ~kManifestInputKindAllBits) != 0u) {
        if (outError) {
            *outError = "Manifest input_kinds contains unsupported bits.";
        }
        return false;
    }
    if (manifest.inputKindsMask == 0u) {
        if (outError) {
            *outError = "Manifest input_kinds must include at least one input kind.";
        }
        return false;
    }

    if (outError) {
        outError->clear();
    }
    return true;
}

} // namespace mousefx::wasm
