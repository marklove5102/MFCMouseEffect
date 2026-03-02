#!/usr/bin/env bash
set -euo pipefail

STRICT=0
if [[ "${1:-}" == "--strict" ]]; then
  STRICT=1
fi

ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT_DIR"

FAILURES=0

check_file() {
  local file="$1"
  local limit="$2"

  if [[ ! -f "$file" ]]; then
    echo "[WARN] missing: $file"
    return
  fi

  local lines
  lines="$(wc -l < "$file" | tr -d ' ')"
  if (( lines > limit )); then
    echo "[OVER] $file: ${lines} lines (limit ${limit})"
    FAILURES=$((FAILURES + 1))
  else
    echo "[OK]   $file: ${lines} lines (limit ${limit})"
  fi
}

echo "== Doc Hygiene (token-oriented) =="
check_file "AGENTS.md" 260
check_file "docs/agent-context/current.md" 220
check_file "docs/README.md" 140
check_file "docs/README.zh-CN.md" 140

echo
echo "== Markdown inventory =="
for dir in docs docs/architecture docs/refactoring docs/issues docs/archive; do
  if [[ -d "$dir" ]]; then
    count="$(find "$dir" -type f -name '*.md' | wc -l | tr -d ' ')"
    echo "$dir: $count"
  fi
done

echo
echo "== Largest markdown files (top 15) =="
find docs -type f -name '*.md' -print0 \
  | xargs -0 wc -c \
  | sort -nr \
  | head -n 15

echo
if (( FAILURES == 0 )); then
  echo "Doc hygiene result: PASS"
  exit 0
fi

if (( STRICT == 1 )); then
  echo "Doc hygiene result: FAIL (${FAILURES} over-limit files)"
  exit 1
fi

echo "Doc hygiene result: WARN (${FAILURES} over-limit files; run with --strict to fail)"
exit 0
