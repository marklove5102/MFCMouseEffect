#pragma once

#include <string>

namespace mousefx {

struct AppShellStartOptions {
    bool showTrayIcon = true;
    std::wstring singleInstanceKey = L"Global\\MFCMouseEffect_SingleInstance_Mutex";
    bool enableRuntimeDiagnostics = false;
};

} // namespace mousefx
