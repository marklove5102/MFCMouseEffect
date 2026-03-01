# Phase 56zzi - macOS ObjC++ Compile-Mode Mechanism Removal

## Scope
- Capability: macOS build/risk-control closure (`Swift-first` enforcement).
- Goal:
  - remove residual ObjC++ compile-mode machinery from macOS CMake.
  - upgrade ObjC++ surface gate from "allowlist shape check" to "hard ban check".

## Decision
- Since all macOS active paths are already Swift/C++-owned, keep no fallback allowlist block in build wiring.
- Enforce stricter regression rules:
  - `.mm` must remain `0`.
  - mac CMake must not contain ObjC++ allowlist block.
  - mac CMake must not contain `COMPILE_FLAGS "-x objective-c++"` or ObjC++ compile-mode selectors.

## Code Changes
1. Removed ObjC++ compile-mode block from:
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/CMakeLists.txt`
  - deleted `MFX_MACOS_OBJCXX_SOURCE_ALLOWLIST`.
  - deleted per-source loop and `set_source_files_properties(... COMPILE_FLAGS "-x objective-c++")`.

2. Tightened ObjC++ surface gate:
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/run-macos-objcxx-surface-regression.sh`
  - fail if allowlist block exists.
  - fail if allowlist source loop exists.
  - fail if `-x objective-c++` compile flag exists.
  - fail if Objective-C++ compile-rule markers remain in mac CMake.

## Verification
- `cmake --build /tmp/mfx-platform-macos-build --target mfx_entry_posix_host -j8`
- `./tools/platform/regression/run-macos-objcxx-surface-regression.sh`
- `./tools/platform/regression/run-posix-scaffold-regression.sh --platform auto`
- `./tools/platform/regression/run-posix-core-effects-contract-regression.sh --platform auto`

All commands passed on macOS.

## Result
- macOS CMake no longer has ObjC++ allowlist or compile-flag routing.
- ObjC++ regression gate now blocks any reintroduction of that mechanism.
