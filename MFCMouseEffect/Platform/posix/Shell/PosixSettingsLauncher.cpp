#include "Platform/posix/Shell/PosixSettingsLauncher.h"

#include <cerrno>
#include <cstdlib>
#include <filesystem>
#include <fcntl.h>
#include <fstream>
#include <spawn.h>
#include <string_view>
#include <sys/wait.h>
#include <system_error>
#include <unistd.h>

extern char** environ;

namespace mousefx {

namespace {

constexpr std::string_view kLaunchCaptureFileEnv = "MFX_TEST_SETTINGS_LAUNCH_CAPTURE_FILE";

bool IsLaunchInputValid(const std::string& url) {
    if (url.empty()) {
        return false;
    }

    for (unsigned char c : url) {
        if (c < 0x20 || c == 0x7f) {
            return false;
        }
    }

    return true;
}

bool EnsureParentDirectory(const std::filesystem::path& filePath) {
    const std::filesystem::path parentPath = filePath.parent_path();
    if (parentPath.empty()) {
        return true;
    }
    std::error_code ec;
    std::filesystem::create_directories(parentPath, ec);
    return !ec;
}

std::string ReadLaunchCaptureFilePath() {
    const char* filePath = std::getenv(kLaunchCaptureFileEnv.data());
    if (filePath == nullptr || filePath[0] == '\0') {
        return {};
    }
    return filePath;
}

bool WriteLaunchCaptureFile(const std::string& filePath, const char* command, const std::string& url) {
    if (filePath.empty() || command == nullptr || command[0] == '\0') {
        return false;
    }

    const std::filesystem::path targetPath(filePath);
    if (!EnsureParentDirectory(targetPath)) {
        return false;
    }

    const std::string tmpPath = filePath + ".tmp";
    {
        std::ofstream out(tmpPath, std::ios::out | std::ios::trunc);
        if (!out.is_open()) {
            return false;
        }
        out << "command=" << command << '\n';
        out << "url=" << url << '\n';
        out << "captured=1\n";
        out.flush();
        if (!out.good()) {
            return false;
        }
    }

    std::error_code ec;
    std::filesystem::rename(tmpPath, targetPath, ec);
    if (!ec) {
        return true;
    }

    std::filesystem::remove(targetPath, ec);
    ec.clear();
    std::filesystem::rename(tmpPath, targetPath, ec);
    return !ec;
}

} // namespace

bool LaunchUrlWithPosixCommand(const char* command, const std::string& url) {
    if (command == nullptr || command[0] == '\0') {
        return false;
    }
    if (!IsLaunchInputValid(url)) {
        return false;
    }

    const std::string captureFilePath = ReadLaunchCaptureFilePath();
    if (!captureFilePath.empty()) {
        return WriteLaunchCaptureFile(captureFilePath, command, url);
    }

    char* const argv[] = {
        const_cast<char*>(command),
        const_cast<char*>(url.c_str()),
        nullptr,
    };

    posix_spawn_file_actions_t fileActions;
    const bool fileActionsReady = posix_spawn_file_actions_init(&fileActions) == 0;

    const int devNullFd = open("/dev/null", O_WRONLY);
    if (fileActionsReady && devNullFd >= 0) {
        posix_spawn_file_actions_adddup2(&fileActions, devNullFd, STDOUT_FILENO);
        posix_spawn_file_actions_adddup2(&fileActions, devNullFd, STDERR_FILENO);
    }

    pid_t childPid = 0;
    const int spawnResult = posix_spawnp(
        &childPid,
        command,
        fileActionsReady ? &fileActions : nullptr,
        nullptr,
        argv,
        ::environ);
    if (fileActionsReady) {
        posix_spawn_file_actions_destroy(&fileActions);
    }
    if (devNullFd >= 0) {
        close(devNullFd);
    }

    if (spawnResult != 0) {
        return false;
    }

    int status = 0;
    pid_t waitResult = -1;
    do {
        waitResult = waitpid(childPid, &status, 0);
    } while (waitResult == -1 && errno == EINTR);

    return waitResult == childPid && WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

} // namespace mousefx
