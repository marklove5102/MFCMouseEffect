#include "Platform/posix/Shell/PosixSettingsLauncher.Internal.h"

#include <cerrno>
#include <fcntl.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

extern char** environ;

namespace mousefx {

bool SpawnSettingsCommand(const char* command, const std::string& target) {
    char* const argv[] = {
        const_cast<char*>(command),
        const_cast<char*>(target.c_str()),
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
