# Windows Manual Handoff

## Purpose
- Single synced handoff note for the current Windows-side manual action.
- Open this file from the Windows synced workspace when I ask for manual validation or command execution on Windows.

## Current Task
### Goal
- Verify the Windows synced workspace at `F:\language\cpp\code\MFCMouseEffect` can see the latest `mfx` build/package command changes.

### Windows Working Path
- `F:\language\cpp\code\MFCMouseEffect`

### Steps
1. Open `cmd` or PowerShell.
2. Run:
   - `cd /d F:\language\cpp\code\MFCMouseEffect`
   - `dir /a tools\platform\package`
   - `dir /a tools\platform\build`
3. Run:
   - `type mfx | findstr build`
   - `type tools\platform\package\build-windows-installer.sh | findstr build-windows-project`
4. If the files are present, run:
   - `.\mfx.cmd build`

### Expected Result
- `tools\platform\package` contains `build-windows-installer.sh`
- `tools\platform\build` contains `build-windows-project.sh`
- `mfx` output contains `build [platform options...]`
- `build-windows-installer.sh` output contains `build-windows-project`
- `.\mfx.cmd build` starts the default Windows no-GPU build flow

### Send Back If It Fails
- Paste the exact output of:
  - `dir /a tools\platform\package`
  - `dir /a tools\platform\build`
  - `type mfx | findstr build`
  - `type tools\platform\package\build-windows-installer.sh | findstr build-windows-project`
