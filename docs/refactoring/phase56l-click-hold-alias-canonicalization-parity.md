# Phase 56l - Click/Hold Alias Canonicalization Parity

## Goal
Improve cross-platform type-semantic consistency by canonicalizing legacy click/hold aliases in shared normalization paths.

## Change

### Click alias normalization
- Updated:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Core/Effects/ClickEffectCompute.cpp`
- New canonical mappings:
  - `icon` / `icon_star` -> `star`
  - `textclick` / `text_click` -> `text`
  - keep `TEXT` -> `text` through lowercase normalization

### Hold alias canonicalization
- Updated:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Effects/HoldRouteCatalog.h`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Effects/HoldRouteCatalog.cpp`
- New canonical mappings:
  - `scifi3d` -> `hologram`
  - `neon3d` -> `hold_neon3d`
  - existing `hold_neon3d_gpu_v2` -> `hold_quantum_halo_gpu_v2`
- Normalization now lowercases input before alias resolution.

### Contract coverage extension
- Updated:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/SettingsStateMapper.EffectsProfileStateBuilder.Macos.cpp`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/lib/core_http_automation_contract_effect_overlay_checks.sh`
- `alias_matrix` and regression assertions now cover:
  - click: `icon`, `textclick`
  - hold: `scifi3d`, `neon3d`

## Validation
- `./tools/platform/regression/run-posix-core-effects-contract-regression.sh --platform auto`
  - passed

## Risk
- Low:
  - canonicalization only; no API removal.
  - improves backward compatibility for legacy type strings.
