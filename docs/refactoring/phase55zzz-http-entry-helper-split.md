# Phase 55zzz: Scaffold HTTP Entry Helper Split

## Capability
- Regression infrastructure

## Why
- `http.sh` combined entry lifecycle management with route assertions.
- Splitting entry helpers keeps the scaffold HTTP checks focused and easier to extend.

## Scope
- Preserve all existing HTTP check behavior and assertions.
- Move entry start/stop lifecycle into a dedicated helper module.

## Code Changes
1. Entry lifecycle helpers:
   - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/lib/http_entry_helpers.sh`
2. Scaffold HTTP checks now source helper:
   - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/lib/http.sh`

## Validation
- `bash -n /Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/lib/http_entry_helpers.sh`
- `bash -n /Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/lib/http.sh`
- `./tools/platform/regression/run-posix-regression-suite.sh --platform auto`

## Contract Impact
- No API/schema/runtime behavior change.
