# Windows Launch-at-Startup Contract

## Scope
- Platform lane: Windows native C++ host.
- User entry: Web settings `launch_at_startup` toggle.
- Backend chain: `CommandHandler.ApplySettings -> AppController::SetLaunchAtStartup -> PlatformLaunchAtStartup`.

## Current Contract
- Windows now supports launch-at-startup natively in C++.
- Implementation uses the per-user registry key:
  - `HKCU\Software\Microsoft\Windows\CurrentVersion\Run`
  - value name: `MFCMouseEffect`
- The stored command is the fully quoted current executable path.
- No installer or elevated privilege is required because the registration is per-user.

## Apply vs Sync Semantics
- `ConfigureLaunchAtStartup(enabled)`:
  - explicit user toggle path
  - writes or deletes the Run value immediately
- `SyncLaunchAtStartupManifest(enabled)`:
  - normal app startup reconciliation path
  - on Windows this is intentionally the same idempotent registry reconciliation as `Configure`
- Reason:
  - Windows Run registration has no separate manifest/runtime split like macOS LaunchAgent.
  - Keeping both entrypoints mapped to the same reconciliation logic preserves the cross-platform controller contract without inventing fake Windows-only state.

## Expected Behavior
- When enabled:
  - next login starts the current `MFCMouseEffect.exe`
- When disabled:
  - the `Run` entry is removed
- If the executable path changes:
  - next normal app launch rewrites the Run entry to the current executable path

## Error Contract
- Common error keys:
  - `launch_at_startup_executable_path_unavailable`
  - `launch_at_startup_registry_access_denied`
  - `launch_at_startup_registry_update_failed`

## Validation
- Build:
  - `MFCMouseEffect/MFCMouseEffect.vcxproj`
  - `Release|x64`
- Manual verification:
  1. Enable `launch_at_startup` in Web settings.
  2. Confirm `HKCU\Software\Microsoft\Windows\CurrentVersion\Run\MFCMouseEffect` exists and points to the current exe.
  3. Disable the toggle.
  4. Confirm the value is removed.
