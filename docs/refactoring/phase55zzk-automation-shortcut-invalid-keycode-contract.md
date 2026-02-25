# Phase 55zzk: Automation Shortcut Invalid-Keycode Contract

## Capability
- 手势映射（Automation）

## Why
- `test-shortcut-from-mac-keycode` covered valid keycode mappings (`Cmd+V`, `Cmd+Tab`), but invalid keycode behavior had no explicit contract.
- Without a negative-path contract, invalid inputs could accidentally produce pseudo shortcut text and cause misleading diagnostics.

## Scope
- Keep valid keycode mapping behavior unchanged.
- Define explicit response contract for invalid/unmapped keycodes.
- Add regression checks for the negative path.

## Code Changes

### 1) API response contract hardening
- Updated:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/WebSettingsServer.TestAutomationShortcutApiRoutes.cpp`
- Behavior:
  - when keycode is invalid/unmapped:
    - `supported=false`
    - `vk_code=0`
    - `shortcut=""`
    - `reason="invalid_or_unmapped_keycode"`

### 2) Regression coverage expansion
- Updated:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/lib/core_http_automation_contract_checks.sh`
- Added assertions for invalid keycode request (`mac_key_code=-1`):
  - status and `ok` unchanged
  - `supported=false`
  - `vk_code=0`
  - `reason` value
  - empty shortcut text

## Validation
- `./tools/platform/regression/run-posix-regression-suite.sh --platform auto`
- `./tools/docs/doc-hygiene-check.sh --strict`

## Contract Impact
- No changes to valid shortcut mapping.
- Invalid keycode path now has deterministic, script-guarded response semantics.
