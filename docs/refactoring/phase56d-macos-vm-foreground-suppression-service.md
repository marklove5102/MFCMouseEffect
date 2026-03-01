# phase56d: macos vm foreground suppression service

## Why
- Foreground suppression was windows-only (`Win32VmForegroundSuppressionService`).
- macOS lacked equivalent suppression and used null service.

## Changes
- Added macOS suppression service:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/System/MacosVmForegroundSuppressionService.h`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/System/MacosVmForegroundSuppressionService.cpp`
- Service behavior:
  - checks active foreground process name;
  - token match against VM/remote viewer keywords;
  - 800ms cache window to avoid excessive polling.
- Added runtime switch:
  - env `MFX_VM_FOREGROUND_SUPPRESSION=0|false|off|no|disable|disabled` disables suppression.
- Wired factory:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/PlatformSystemServicesFactory.cpp`
  - macOS now returns `MacosVmForegroundSuppressionService`.
- Build wiring:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/CMakeLists.txt`.

## Validation
- `cmake --build /tmp/mfx-platform-macos-build --target mfx_entry_posix_host -j8`
- `./tools/platform/regression/run-posix-core-automation-contract-regression.sh --platform auto --build-dir /tmp/mfx-platform-macos-build`
- Result: passed.
