# MFCMouseEffect

Application for global mouse click effects (ripples).

## Launch Modes

The application supports two launch modes via command line arguments:

### 1. Tray Mode (Default)
Shows a system tray icon for management (Exit option).
```bash
MFCMouseEffect.exe
# OR
MFCMouseEffect.exe -mode tray
```

### 2. Background Mode
Runs fully in the background without any tray icon. Useful when managed by a parent process.
```bash
MFCMouseEffect.exe -mode background
```
In this mode, the parent process is responsible for terminating the application.
