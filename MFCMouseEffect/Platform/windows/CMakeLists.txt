if(TARGET mfx_shell_windows)
    return()
endif()

add_library(mfx_shell_windows STATIC
    "${MFX_PROJECT_ROOT}/Platform/windows/Shell/Win32AppShell.cpp"
    "${MFX_PROJECT_ROOT}/Platform/windows/Shell/Win32DpiAwarenessService.cpp"
    "${MFX_PROJECT_ROOT}/Platform/windows/Shell/Win32EventLoopService.cpp"
    "${MFX_PROJECT_ROOT}/Platform/windows/Shell/Win32ShellServicesFactory.cpp"
    "${MFX_PROJECT_ROOT}/Platform/windows/Shell/Win32SettingsLauncher.cpp"
    "${MFX_PROJECT_ROOT}/Platform/windows/Shell/Win32SingleInstanceGuard.cpp"
    "${MFX_PROJECT_ROOT}/Platform/windows/Shell/Win32TrayService.cpp"
    "${MFX_PROJECT_ROOT}/Platform/windows/Shell/Win32UserNotificationService.cpp"
    "${MFX_PROJECT_ROOT}/Platform/windows/Shell/Tray/Win32TrayHostWindow.cpp"
    "${MFX_PROJECT_ROOT}/Platform/windows/Shell/Tray/Win32TrayMenuBuilder.cpp"
)

target_link_libraries(mfx_shell_windows PUBLIC mfx_shell_contracts)
target_compile_features(mfx_shell_windows PUBLIC cxx_std_17)
target_compile_definitions(mfx_shell_windows
    PRIVATE
        MFX_TARGET_PLATFORM_WINDOWS=1
        _WINDOWS
        UNICODE
        _UNICODE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
)

message(STATUS "Configured shell package target: mfx_shell_windows")
