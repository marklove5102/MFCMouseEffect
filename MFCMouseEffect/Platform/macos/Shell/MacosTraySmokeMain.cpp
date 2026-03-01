#include "Platform/macos/Shell/MacosEventLoopService.h"
#include "Platform/macos/Shell/MacosSettingsLauncher.h"
#include "Platform/macos/Shell/MacosTrayService.h"
#include "Platform/posix/Shell/PosixKeyValueCaptureFile.h"

#include "MouseFx/Core/Shell/IAppShellHost.h"

#import <dispatch/dispatch.h>

#include <cstdlib>
#include <cstdio>
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>

namespace {

char ToLowerAscii(char c) {
    if (c >= 'A' && c <= 'Z') {
        return static_cast<char>(c - 'A' + 'a');
    }
    return c;
}

bool EqualsIgnoreCaseAscii(std::string_view lhs, std::string_view rhs) {
    if (lhs.size() != rhs.size()) {
        return false;
    }
    for (size_t i = 0; i < lhs.size(); ++i) {
        if (ToLowerAscii(lhs[i]) != ToLowerAscii(rhs[i])) {
            return false;
        }
    }
    return true;
}

bool ReadBoolEnv(const char* key) {
    if (key == nullptr || key[0] == '\0') {
        return false;
    }
    const char* raw = std::getenv(key);
    if (raw == nullptr || raw[0] == '\0') {
        return false;
    }
    const std::string_view value(raw);
    return value == "1" ||
           EqualsIgnoreCaseAscii(value, "true") ||
           EqualsIgnoreCaseAscii(value, "yes") ||
           EqualsIgnoreCaseAscii(value, "on");
}

std::string ReadStringEnvOrDefault(const char* key, const char* defaultValue) {
    const char* raw = std::getenv(key);
    if (raw == nullptr || raw[0] == '\0') {
        return (defaultValue == nullptr) ? std::string() : std::string(defaultValue);
    }
    return std::string(raw);
}

class TraySmokeHost final : public mousefx::IAppShellHost {
public:
    TraySmokeHost(
        mousefx::MacosEventLoopService* loop,
        bool expectSettingsAction,
        std::string settingsUrl,
        std::string launchCaptureFilePath)
        : loop_(loop),
          expectSettingsAction_(expectSettingsAction),
          settingsUrl_(std::move(settingsUrl)),
          launchCaptureFilePath_(std::move(launchCaptureFilePath)) {}

    mousefx::AppController* AppControllerForShell() noexcept override {
        return nullptr;
    }

    void OpenSettingsFromShell() override {
        ++settingsActionCount_;
        settingsLaunchOk_ = settingsLauncher_.OpenUrlUtf8(settingsUrl_);
        if (expectSettingsAction_ && loop_ != nullptr) {
            loop_->RequestExit();
        }
    }

    void RequestExitFromShell() override {
        if (loop_ != nullptr) {
            loop_->RequestExit();
        }
    }

    bool HasSettingsAction() const {
        return settingsActionCount_ > 0;
    }

    bool SettingsLaunchOk() const {
        return settingsLaunchOk_;
    }

    const std::string& SettingsUrl() const {
        return settingsUrl_;
    }
    const std::string& LaunchCaptureFilePath() const {
        return launchCaptureFilePath_;
    }

private:
    mousefx::MacosEventLoopService* loop_ = nullptr;
    bool expectSettingsAction_ = false;
    std::string settingsUrl_{};
    std::string launchCaptureFilePath_{};
    mousefx::MacosSettingsLauncher settingsLauncher_{};
    bool settingsLaunchOk_ = false;
    size_t settingsActionCount_ = 0;
};

} // namespace

int main(int argc, char* argv[]) {
    const bool expectSettingsAction = ReadBoolEnv("MFX_TEST_TRAY_SMOKE_EXPECT_SETTINGS_ACTION");
    std::string settingsUrl = ReadStringEnvOrDefault(
        "MFX_TEST_TRAY_SMOKE_SETTINGS_URL",
        "http://127.0.0.1:9527/?token=tray-smoke");
    std::string launchCaptureFilePath = ReadStringEnvOrDefault(
        "MFX_TEST_SETTINGS_LAUNCH_CAPTURE_FILE",
        "");

    bool forceExpectSettingsAction = false;
    for (int i = 1; i < argc; ++i) {
        const std::string_view arg(argv[i]);
        if (arg == "--expect-settings-action") {
            forceExpectSettingsAction = true;
            continue;
        }
        if (arg == "--settings-url") {
            if (i + 1 >= argc) {
                std::fprintf(stderr, "mfx_shell_macos_tray_smoke: missing value for --settings-url\n");
                return 64;
            }
            settingsUrl = argv[++i];
            continue;
        }
        if (arg == "--launch-capture-file") {
            if (i + 1 >= argc) {
                std::fprintf(stderr, "mfx_shell_macos_tray_smoke: missing value for --launch-capture-file\n");
                return 64;
            }
            launchCaptureFilePath = argv[++i];
            continue;
        }
        std::fprintf(stderr, "mfx_shell_macos_tray_smoke: unknown argument: %.*s\n", static_cast<int>(arg.size()), arg.data());
        return 64;
    }

    const bool expectSettingsActionEffective = expectSettingsAction || forceExpectSettingsAction;
    if (!launchCaptureFilePath.empty()) {
        setenv("MFX_TEST_SETTINGS_LAUNCH_CAPTURE_FILE", launchCaptureFilePath.c_str(), 1);
    }

    mousefx::MacosEventLoopService loop;
    mousefx::MacosTrayService tray;
    TraySmokeHost host(&loop, expectSettingsActionEffective, settingsUrl, launchCaptureFilePath);
    TraySmokeHost* hostPtr = &host;

    if (!tray.Start(&host, true)) {
        std::fprintf(stderr, "mfx_shell_macos_tray_smoke: failed to start tray service\n");
        return 2;
    }

    if (expectSettingsActionEffective) {
        dispatch_after(
            dispatch_time(DISPATCH_TIME_NOW, static_cast<int64_t>(120) * NSEC_PER_MSEC),
            dispatch_get_main_queue(),
            ^{
              if (hostPtr != nullptr) {
                  hostPtr->OpenSettingsFromShell();
              }
            });
    }
    dispatch_after(
        dispatch_time(DISPATCH_TIME_NOW, static_cast<int64_t>(800) * NSEC_PER_MSEC),
        dispatch_get_main_queue(),
        ^{
          if (hostPtr != nullptr) {
              hostPtr->RequestExitFromShell();
          }
        });

    const int code = loop.Run();
    tray.Stop();
    if (code != 0) {
        return code;
    }

    if (expectSettingsActionEffective && !host.HasSettingsAction()) {
        std::fprintf(stderr, "mfx_shell_macos_tray_smoke: settings action was not triggered\n");
        return 3;
    }
    if (expectSettingsActionEffective && !host.SettingsLaunchOk()) {
        std::fprintf(
            stderr,
            "mfx_shell_macos_tray_smoke: settings launch failed for url: %s\n",
            host.SettingsUrl().c_str());
        return 4;
    }
    if (expectSettingsActionEffective && !host.LaunchCaptureFilePath().empty()) {
        const bool wrote = mousefx::WritePosixKeyValueCaptureFile(
            host.LaunchCaptureFilePath(),
            {
                {"command", "open"},
                {"url", host.SettingsUrl()},
                {"captured", "1"},
            });
        if (!wrote) {
            std::fprintf(
                stderr,
                "mfx_shell_macos_tray_smoke: failed to write launch capture file: %s\n",
                host.LaunchCaptureFilePath().c_str());
            return 5;
        }
    }
    return 0;
}
