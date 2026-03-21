#include "pch.h"

#include "Platform/windows/System/Win32LaunchAtStartup.h"

#include <string>
#include <vector>

#pragma comment(lib, "Advapi32.lib")

namespace mousefx::platform::windows {

namespace {

constexpr wchar_t kRunKeyPath[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
constexpr wchar_t kRunValueName[] = L"MFCMouseEffect";

std::wstring GetExecutablePath() {
    std::vector<wchar_t> buffer(MAX_PATH, L'\0');
    while (true) {
        const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (length == 0) {
            return {};
        }
        if (length < buffer.size() - 1) {
            return std::wstring(buffer.data(), length);
        }
        buffer.resize(buffer.size() * 2, L'\0');
    }
}

std::wstring BuildStartupCommand() {
    const std::wstring exePath = GetExecutablePath();
    if (exePath.empty()) {
        return {};
    }

    std::wstring command;
    command.reserve(exePath.size() + 2);
    command.push_back(L'"');
    command.append(exePath);
    command.push_back(L'"');
    return command;
}

bool SetErrorFromCode(LSTATUS status, std::string* outError) {
    if (!outError) {
        return false;
    }

    switch (status) {
    case ERROR_SUCCESS:
        outError->clear();
        return true;
    case ERROR_ACCESS_DENIED:
        *outError = "launch_at_startup_registry_access_denied";
        return false;
    case ERROR_FILE_NOT_FOUND:
        *outError = "launch_at_startup_registry_value_missing";
        return false;
    default:
        *outError = "launch_at_startup_registry_update_failed";
        return false;
    }
}

bool WriteRunValue(const std::wstring& command, std::string* outError) {
    HKEY runKey = nullptr;
    const LSTATUS openStatus = RegCreateKeyExW(
        HKEY_CURRENT_USER,
        kRunKeyPath,
        0,
        nullptr,
        REG_OPTION_NON_VOLATILE,
        KEY_SET_VALUE,
        nullptr,
        &runKey,
        nullptr);
    if (openStatus != ERROR_SUCCESS) {
        SetErrorFromCode(openStatus, outError);
        return false;
    }

    const DWORD byteCount = static_cast<DWORD>((command.size() + 1) * sizeof(wchar_t));
    const LSTATUS writeStatus = RegSetValueExW(
        runKey,
        kRunValueName,
        0,
        REG_SZ,
        reinterpret_cast<const BYTE*>(command.c_str()),
        byteCount);
    RegCloseKey(runKey);

    if (writeStatus != ERROR_SUCCESS) {
        SetErrorFromCode(writeStatus, outError);
        return false;
    }
    if (outError) {
        outError->clear();
    }
    return true;
}

bool DeleteRunValue(std::string* outError) {
    HKEY runKey = nullptr;
    const LSTATUS openStatus = RegOpenKeyExW(HKEY_CURRENT_USER, kRunKeyPath, 0, KEY_SET_VALUE, &runKey);
    if (openStatus == ERROR_FILE_NOT_FOUND) {
        if (outError) {
            outError->clear();
        }
        return true;
    }
    if (openStatus != ERROR_SUCCESS) {
        SetErrorFromCode(openStatus, outError);
        return false;
    }

    const LSTATUS deleteStatus = RegDeleteValueW(runKey, kRunValueName);
    RegCloseKey(runKey);
    if (deleteStatus == ERROR_FILE_NOT_FOUND) {
        if (outError) {
            outError->clear();
        }
        return true;
    }
    if (deleteStatus != ERROR_SUCCESS) {
        SetErrorFromCode(deleteStatus, outError);
        return false;
    }
    if (outError) {
        outError->clear();
    }
    return true;
}

bool ApplyRunRegistration(bool enabled, std::string* outError) {
    if (outError) {
        outError->clear();
    }

    if (!enabled) {
        return DeleteRunValue(outError);
    }

    const std::wstring command = BuildStartupCommand();
    if (command.empty()) {
        if (outError) {
            *outError = "launch_at_startup_executable_path_unavailable";
        }
        return false;
    }
    return WriteRunValue(command, outError);
}

} // namespace

bool ConfigureLaunchAtStartup(bool enabled, std::string* outError) {
    return ApplyRunRegistration(enabled, outError);
}

bool SyncLaunchAtStartupManifest(bool enabled, std::string* outError) {
    return ApplyRunRegistration(enabled, outError);
}

} // namespace mousefx::platform::windows
