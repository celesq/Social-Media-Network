#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
SUITE_DIR="$(cd -- "$SCRIPT_DIR/.." && pwd)"
REPO_DIR="${1:-$(cd -- "$SUITE_DIR/.." && pwd)}"
RESULTS_DIR="$SCRIPT_DIR/results"

if [[ ! -f "$REPO_DIR/friends.h" || ! -f "$REPO_DIR/Makefile" ]]; then
  echo "ERROR: '$REPO_DIR' does not look like the Social-Media-Network repository root." >&2
  echo "Usage: $0 [repo-root]" >&2
  exit 2
fi

MAX_PEOPLE="$(awk '/^[[:space:]]*#[[:space:]]*define[[:space:]]+MAX_PEOPLE[[:space:]]+[0-9]+/{print $3; exit}' "$REPO_DIR/friends.h")"
if [[ -z "$MAX_PEOPLE" || "$MAX_PEOPLE" -lt 10000 ]]; then
  echo "ERROR: MAX_PEOPLE must be >= 10000 in $REPO_DIR/friends.h (found '${MAX_PEOPLE:-missing}')." >&2
  exit 2
fi

mkdir -p "$RESULTS_DIR"
rm -rf "$SCRIPT_DIR/generated"

python3 "$SCRIPT_DIR/run_benchmarks.py" \
  --repo "$REPO_DIR" \
  --profile scale10k \
  --repeats 5 \
  --no-valgrind \
  --no-sanitizers \
  --output "$RESULTS_DIR/benchmark_10k_scale_results.json"

python3 "$SCRIPT_DIR/summarize_results.py" \
  "$RESULTS_DIR/benchmark_10k_scale_results.json" \
  --output "$RESULTS_DIR/benchmark_10k_scale_summary.md"

cat "$RESULTS_DIR/benchmark_10k_scale_summary.md"
