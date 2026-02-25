# Phase 55zp: P0/P1 Doc Index Compaction

## Why
- Top-level P1 docs were close to size limits and carried long low-value phase lists.
- Frequent phase slices would quickly push `/docs/README*.md` over limits and increase context token overhead.

## Scope
- Compact top-level index docs to retain only high-value navigation entries.
- Keep detailed phase chronology in roadmap status doc, not in first-read indexes.
- Preserve discoverability through targeted retrieval commands and roadmap references.

## Code Changes

### 1) Compact top-level README indexes
- Updated:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/docs/README.md`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/docs/README.zh-CN.md`
- Changes:
  - replaced exhaustive 53x/55x phase doc lists with key-doc subsets.
  - added explicit pointer to roadmap status for full split-chain/hardening chronology.

### 2) Compact active agent context index section
- Updated `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/docs/agent-context/current.md`.
- Changes:
  - replaced exhaustive Phase 55 slice list with key WASM hardening docs + closure docs.
  - added explicit default-read rule: full Phase 55 chronology should be read from roadmap doc on demand.

## Validation
- `./tools/docs/doc-hygiene-check.sh --strict`
- manual check:
  - top-level indexes still include key navigation pointers
  - roadmap status remains authoritative full chronology source

## Contract Impact
- No runtime/API behavior changes.
- Documentation governance improvement only:
  - lower token pressure in first-read docs,
  - better alignment with AI-first layered doc policy.
