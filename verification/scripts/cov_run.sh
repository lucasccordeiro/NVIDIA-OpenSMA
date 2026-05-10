#!/usr/bin/env bash
# cov_run.sh <target> <phase> <esbmc> <flags...> -- <inputs...>
#
# Phase 4 coverage runner. Invokes ESBMC twice if needed:
#   1. --k-path-coverage (PathCrawler-style, primary metric).
#   2. --branch-function-coverage (fallback if k-path emits 0 goals).
#
# Output:
#   results/cov/<target>_<phase>.log       — full ESBMC stdout/stderr
#   results/cov/<target>_<phase>.json      — per-claim JSON (cov-report.json)
#   results/cov/<target>_<phase>.metric    — the chosen metric ("k-path" or
#                                            "branch-fn"), one word.
#
# The two-stage --/-- argv split is to keep flag passthrough unambiguous.
set -euo pipefail

if [[ $# -lt 4 ]]; then
  echo "usage: $0 <target> <phase> <esbmc> <flags...> -- <inputs...>" >&2
  exit 2
fi

target="$1"; shift
phase="$1"; shift
esbmc="$1"; shift

orig_cwd="$(pwd)"

# Resolve a path argument to absolute if it exists as a file/dir relative to
# the original CWD. Leave non-path arguments (e.g. macros, std versions) alone.
abs_if_path() {
  local arg="$1"
  if [[ "$arg" == -I* ]]; then
    local p="${arg#-I}"
    if [[ "$p" != /* && -e "$orig_cwd/$p" ]]; then
      echo "-I$orig_cwd/$p"
    else
      echo "$arg"
    fi
  elif [[ "$arg" != /* && -e "$orig_cwd/$arg" ]]; then
    echo "$orig_cwd/$arg"
  else
    echo "$arg"
  fi
}

flags=()
while [[ $# -gt 0 && "$1" != "--" ]]; do
  flags+=("$(abs_if_path "$1")"); shift
done
[[ $# -gt 0 ]] && shift  # drop the '--'
inputs=()
for arg in "$@"; do
  inputs+=("$(abs_if_path "$arg")")
done

cov_dir="$(cd "$(dirname "$0")/.." && pwd)/results/cov"
work_dir="$cov_dir/work-${target}-${phase}"
log="$cov_dir/${target}_${phase}.log"
json="$cov_dir/${target}_${phase}.json"
metric_file="$cov_dir/${target}_${phase}.metric"
cmd_file="$cov_dir/${target}_${phase}.cmd"

mkdir -p "$work_dir"
rm -f "$log" "$json" "$metric_file" "$cmd_file" "$work_dir/cov-report.json"

# Record the resolved invocation (flags + inputs) so the aggregator can
# extract --unwind and the flag profile reliably from a single source of
# truth — ESBMC's stdout doesn't echo the command line.
{
  printf '%s' "$esbmc"
  for a in "${flags[@]}"; do printf ' %q' "$a"; done
  printf ' --'
  for a in "${inputs[@]}"; do printf ' %q' "$a"; done
  printf '\n'
} > "$cmd_file"

run_esbmc() {
  local mode_flag="$1"
  (cd "$work_dir" && \
     "$esbmc" "${flags[@]}" "$mode_flag" \
       --cov-assume-asserts --cov-report-json \
       "${inputs[@]}") > "$log" 2>&1 || true
  if [[ -f "$work_dir/cov-report.json" ]]; then
    mv -f "$work_dir/cov-report.json" "$json"
  fi
}

# Stage 1: try k-path coverage.
run_esbmc --k-path-coverage

# Detect ESBMC failures distinct from "0 goals reached" — if no [Coverage]
# block was emitted at all, the run failed before instrumentation took
# effect (e.g. flag rejected, parse error). Report as `error` so the
# aggregator can flag the row instead of charting it as 0/0.
if ! grep -q "^\[Coverage\]" "$log"; then
  echo "error" > "$metric_file"
elif grep -qE "k-Path Witnesses[[:space:]]*:[[:space:]]*0\b" "$log"; then
  run_esbmc --branch-function-coverage
  if ! grep -q "^\[Coverage\]" "$log"; then
    echo "error" > "$metric_file"
  else
    echo "branch-fn" > "$metric_file"
  fi
else
  echo "k-path" > "$metric_file"
fi

metric=$(cat "$metric_file")
summary=$(grep -E "k-Path Coverage|Branch Coverage" "$log" | tail -1 || true)
echo "[cov] ${target} ${phase} (${metric}): ${summary:-<no coverage line>}"

# Drop the per-target work-dir once the JSON has been moved out — we don't
# need the empty scratch directory after a successful run.
[[ -d "$work_dir" ]] && rmdir "$work_dir" 2>/dev/null || true
