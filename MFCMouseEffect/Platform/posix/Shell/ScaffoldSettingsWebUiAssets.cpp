#include "pch.h"

#include "Platform/posix/Shell/ScaffoldSettingsWebUiAssets.h"

#include <cctype>
#include <cstdlib>
#include <fstream>
#include <utility>

namespace mousefx::platform::scaffold {
namespace {

bool IsSafeWebPath(const std::string& path) {
    if (path.empty()) {
        return false;
    }
    if (path[0] != '/') {
        return false;
    }
    if (path.find("..") != std::string::npos) {
        return false;
    }
    if (path.find('\\') != std::string::npos) {
        return false;
    }
    return true;
}

std::string ToLowerAsciiCopy(const std::string& text) {
    std::string lowered;
    lowered.reserve(text.size());
    for (char c : text) {
        lowered.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    return lowered;
}

std::string ContentTypeForWebPath(const std::string& path) {
    const std::string lowered = ToLowerAsciiCopy(path);
    const auto endsWith = [&](const char* suffix) {
        const size_t suffixSize = std::char_traits<char>::length(suffix);
        return lowered.size() >= suffixSize &&
               lowered.compare(lowered.size() - suffixSize, suffixSize, suffix) == 0;
    };

    if (endsWith(".html")) return "text/html; charset=utf-8";
    if (endsWith(".js")) return "application/javascript; charset=utf-8";
    if (endsWith(".css")) return "text/css; charset=utf-8";
    if (endsWith(".json")) return "application/json; charset=utf-8";
    if (endsWith(".png")) return "image/png";
    if (endsWith(".svg")) return "image/svg+xml";
    return "application/octet-stream";
}

void AddWebUiDirIfExists(
    const std::filesystem::path& dir,
    std::vector<std::filesystem::path>* outDirs) {
    if (!outDirs || dir.empty()) {
        return;
    }

    std::error_code ec;
    if (!std::filesystem::exists(dir, ec) || ec) {
        return;
    }
    if (!std::filesystem::is_directory(dir, ec) || ec) {
        return;
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

} // namespace

std::vector<std::filesystem::path> BuildWebUiBaseDirs() {
    std::vector<std::filesystem::path> dirs;

    const char* overrideDir = std::getenv("MFX_SCAFFOLD_WEBUI_DIR");
    if (overrideDir && *overrideDir != '\0') {
        AddWebUiDirIfExists(std::filesystem::path(overrideDir), &dirs);
        return dirs;
    }

    std::error_code ec;
    const std::filesystem::path cwd = std::filesystem::current_path(ec);
    if (!ec && !cwd.empty()) {
        AddWebUiDirIfExists(cwd / "MFCMouseEffect" / "WebUI", &dirs);
        AddWebUiDirIfExists(cwd / "WebUI", &dirs);
        AddWebUiDirIfExists(cwd.parent_path() / "MFCMouseEffect" / "WebUI", &dirs);
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
    if (!IsSafeWebPath(requestPath)) {
        return false;
    }

    std::string path = requestPath;
    const size_t queryPos = path.find('?');
    if (queryPos != std::string::npos) {
        path = path.substr(0, queryPos);
    }
    if (path == "/") {
        path = "/index.html";
    }
    if (!IsSafeWebPath(path)) {
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
