#include "pch.h"
#include "WebSettingsServer.h"

#include <cstdlib>
#include <filesystem>
#include <random>
#include <sstream>
#include <vector>

#include "Platform/PlatformRuntimeEnvironment.h"
#include "MouseFx/Server/HttpServer.h"
#include "MouseFx/Server/WebUiAssets.h"

namespace mousefx {
namespace {

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

void AddEnvWebUiDir(std::vector<std::filesystem::path>* outDirs) {
    const char* envDir = std::getenv("MFX_WEBUI_DIR");
    if (envDir == nullptr || envDir[0] == '\0') {
        return;
    }
    AddWebUiDirIfExists(std::filesystem::path(envDir), outDirs);
}

void AddExecutableWebUiDir(std::vector<std::filesystem::path>* outDirs) {
    const std::wstring exeDir = platform::GetExecutableDirectoryW();
    if (exeDir.empty()) {
        return;
    }
    AddWebUiDirIfExists(std::filesystem::path(exeDir) / L"webui", outDirs);
}

void AddWorkingDirectoryWebUiDirs(std::vector<std::filesystem::path>* outDirs) {
    std::error_code ec;
    const std::filesystem::path cwd = std::filesystem::current_path(ec);
    if (ec || cwd.empty()) {
        return;
    }

    AddWebUiDirIfExists(cwd / "MFCMouseEffect" / "WebUI", outDirs);
    AddWebUiDirIfExists(cwd / "WebUI", outDirs);
    AddWebUiDirIfExists(cwd.parent_path() / "MFCMouseEffect" / "WebUI", outDirs);
}

void AddSourceTreeWebUiDir(std::vector<std::filesystem::path>* outDirs) {
    const std::filesystem::path sourcePath(__FILE__);
    if (!sourcePath.is_absolute()) {
        return;
    }
    const std::filesystem::path projectDir =
        sourcePath.parent_path().parent_path().parent_path();
    AddWebUiDirIfExists(projectDir / "WebUI", outDirs);
}

std::wstring ResolveWebUiBaseDir() {
    std::vector<std::filesystem::path> candidates;
    candidates.reserve(8);
    AddEnvWebUiDir(&candidates);
    AddExecutableWebUiDir(&candidates);
    AddWorkingDirectoryWebUiDirs(&candidates);
    AddSourceTreeWebUiDir(&candidates);
    if (candidates.empty()) {
        return {};
    }
    return candidates.front().wstring();
}

} // namespace

WebSettingsServer::WebSettingsServer(AppController* controller) : controller_(controller) {
    RotateToken();
    http_ = std::make_unique<HttpServer>();
    assets_ = std::make_unique<WebUiAssets>(ResolveWebUiBaseDir());
}

WebSettingsServer::~WebSettingsServer() {
    Stop();
}

bool WebSettingsServer::Start() {
    if (!http_) return false;
    if (http_->IsRunning()) return true;

    const bool started = http_->StartLoopback([this](const HttpRequest& req, HttpResponse& resp) {
        HandleRequest(req, resp);
    });
    if (started) {
        Touch();
        StartMonitor();
    }
    return started;
}

void WebSettingsServer::Stop() {
    if (http_) http_->Stop();
    StopMonitor();
}

bool WebSettingsServer::IsRunning() const {
    return http_ && http_->IsRunning();
}

uint16_t WebSettingsServer::Port() const {
    return http_ ? http_->Port() : 0;
}

std::string WebSettingsServer::Url() const {
    std::ostringstream ss;
    ss << "http://127.0.0.1:" << (int)Port() << "/?token=" << TokenCopy();
    return ss.str();
}

std::string WebSettingsServer::MakeToken() {
    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_int_distribution<int> dist(0, 15);
    std::string s;
    s.reserve(32);
    for (int i = 0; i < 32; ++i) {
        int v = dist(rng);
        s.push_back(v < 10 ? (char)('0' + v) : (char)('a' + (v - 10)));
    }
    return s;
}

std::string WebSettingsServer::Token() const {
    return TokenCopy();
}

std::string WebSettingsServer::TokenCopy() const {
    std::lock_guard<std::mutex> lock(tokenMutex_);
    return token_;
}

} // namespace mousefx
