if(TARGET mfx_shell_linux)
    return()
endif()

if(NOT TARGET mfx_shell_posix_common)
    message(FATAL_ERROR "mfx_shell_posix_common is required before configuring mfx_shell_linux.")
endif()

add_library(mfx_shell_linux STATIC
    "${MFX_PROJECT_ROOT}/Platform/linux/Shell/LinuxEventLoopService.cpp"
    "${MFX_PROJECT_ROOT}/Platform/linux/Shell/LinuxShellServicesFactory.cpp"
    "${MFX_PROJECT_ROOT}/Platform/linux/Shell/LinuxSettingsLauncher.cpp"
    "${MFX_PROJECT_ROOT}/Platform/linux/Shell/LinuxSingleInstanceGuard.cpp"
    "${MFX_PROJECT_ROOT}/Platform/linux/Shell/LinuxTrayService.cpp"
    "${MFX_PROJECT_ROOT}/Platform/linux/Shell/LinuxUserNotificationService.cpp"
)

target_link_libraries(mfx_shell_linux
    PUBLIC
        mfx_shell_contracts
        mfx_shell_posix_common
)
target_compile_features(mfx_shell_linux PUBLIC cxx_std_17)
target_compile_definitions(mfx_shell_linux PRIVATE MFX_TARGET_PLATFORM_LINUX=1)

message(STATUS "Configured shell package target: mfx_shell_linux")
