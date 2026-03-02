# Text Layout Garbled & Config Path Fixes

## Problem Description
1. **Garbled Text**: `config.json` containing Chinese characters ("美丽", "健康") caused display issues due to encoding mismatches (GBK vs UTF-8).
2. **Confusing Config Path**: In Debug mode, the application was reading from `%AppData%/MFCMouseEffect/config.json` while the user was editing the local `config.json` in their project directory, causing confusion as changes didn't reflect.

## Changes Implemented

### 1. Debug Mode Config Priority
Modified `AppController.cpp` to prioritize the **local executable directory** over `%AppData%` when compiling in `_DEBUG` mode.
- **Debug**: Reads/Writes `[ExeDir]\config.json`.
- **Release**: Reads/Writes `%AppData%\MFCMouseEffect\config.json` (fallback to `[ExeDir]` if AppData fails).
- This ensures developers see changes immediately when editing the local file.

### 2. Robust Encoding Support
Modified `EffectConfig.cpp` to implement `ReadFileAsUtf8`:
- Detects if the file is valid UTF-8.
- If not (e.g., GBK/ANSI), essentially converts it from System Code Page (CP_ACP) to UTF-8 before parsing.
- Handles BOM (Byte Order Mark) correctly.

### 3. Auto-Restoration
Modified `EffectConfig.cpp` to:
- If `config.json` is missing or empty, create a fresh one with defaults.
- If `config.json` is corrupted (JSON parse error), **overwrite it** with a fresh default config.

## Verification
- **Debug Session**: Run the app. Check Output window for `MouseFx: Loading config from ...`. Verify it points to the local path.
- **Encoding Test**: Save `config.json` as ANSI/GBK. Run app. Text should display correctly (no "乱码").
