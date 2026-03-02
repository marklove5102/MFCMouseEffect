# Phase 43: POSIX Scaffold Regression Script

## Top Decisions (Important)
- Add one command to verify POSIX scaffold changes on host (macOS/Linux) with stable, repeatable checks.
- Keep script architecture modular: orchestration file + single-responsibility check modules.
- Preserve existing manual validation semantics, but codify them as executable checks.

## New Files
- `tools/platform/regression/run-posix-scaffold-regression.sh`
- `tools/platform/regression/lib/common.sh`
- `tools/platform/regression/lib/build.sh`
- `tools/platform/regression/lib/smoke.sh`
- `tools/platform/regression/lib/http.sh`

## Responsibility Split
- `run-posix-scaffold-regression.sh`
  - argument parsing
  - host platform resolve (`auto`)
  - step orchestration
- `lib/build.sh`
  - configure and build targets by platform package
- `lib/smoke.sh`
  - background stdin exit compatibility checks
  - macOS smoke executable checks
- `lib/http.sh`
  - scaffold HTTP regression matrix (default/custom/missing-webui)
- `lib/common.sh`
  - assertions, logging, command requirements, HTTP status helper

## Behavior / Compatibility
- No product runtime behavior changed.
- Verification now has a deterministic entry command for POSIX scaffold-related changes.

## Validation
- Executed:
  - `./tools/platform/regression/run-posix-scaffold-regression.sh --platform macos --build-dir /tmp/mfx-platform-macos-build`
- Result:
  - build passed
  - smoke checks passed
  - HTTP regression checks passed

## Follow-up
- Completed in Phase 44:
  - moved shell service assembly into package-local factories (`windows/macos/linux`) so top-level shell services factory is dispatch-only.
