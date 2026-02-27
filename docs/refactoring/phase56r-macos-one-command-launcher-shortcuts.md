# Phase 56r - macOS One-Command Launcher Shortcuts

## Why
- Manual testing flow required repeatedly copying long script paths and options.
- For daily macOS iteration, the command surface should be short and stable.

## What Changed
1. Added repo-root shortcut script:
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/mfx`

2. Supported commands:
- `./mfx start`  
  Build + run macOS core host (default auto stop 30 minutes).
- `./mfx fast`  
  Run without rebuild (default auto stop 30 minutes).
- `./mfx effects`  
  Run macOS effects type parity selfcheck.

3. Added option:
- `--minutes N` for `start`/`fast`.

4. Docs updated:
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/README.md`
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/docs/README.md`
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/docs/README.zh-CN.md`

## Validation
```bash
bash -n ./mfx
./mfx help
```
