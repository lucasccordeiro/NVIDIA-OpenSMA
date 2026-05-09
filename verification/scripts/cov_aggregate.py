#!/usr/bin/env python3
"""Aggregate Phase 4 coverage results into TSV / Markdown.

Reads results/cov/<target>_<phase>.{json,log,metric} written by cov_run.sh
and emits one row per (target, phase) plus a per-function rollup.

Usage:
    cov_aggregate.py <cov_dir>              # TSV to stdout
    cov_aggregate.py --markdown <cov_dir>   # Markdown to stdout

Flag profile family is inferred from the .log header (best-effort) and
written into the `flags` column. Exit-edge claims (e.g. `i >= N` at loop
heads) are counted into `exit_uncov`; effective_ratio = reached /
max(1, total - exit_uncov).
"""
from __future__ import annotations

import argparse
import json
import re
import sys
from collections import defaultdict
from pathlib import Path

# Modules in scope (Phase 4 shortlist). Keep in sync with PHASE4_PLAN.md.
TARGETS = [
    "nsm_dcd_event_handlers",
    "soc_sma_filter",
    "debug_telemetry_sma",
    "pca9555",
    "ntc_table",
    "mctp_validator",
    "nsm_bitmask",
    "mctp_packet",
    "mctp_router",
    "nsm_type5_validate",
]

# Modules with no Phase 2 (k-induction) target — covered at p1 only.
# c2c_mailbox is trivially total (no loops/memcpy → no _func rule).
# ssif_safety's data path Phase 2 is blocked alongside its Phase 1 by
# esbmc#4448 (bit_cast pointer-bound loss); only the narrow Phase 1
# baseline has a coverage measurement.
TARGETS_P1_ONLY = [
    "c2c_mailbox",
    "ssif_safety",
]
PHASES = ["p1", "p2"]


def counts_from_claims(claims: list[dict]) -> tuple[int, int]:
    """Ground truth for (reached, total). The JSON reflects the final
    per-claim verdict; the log's [Coverage] block is unreliable under
    --k-induction, which emits one block per inductive step (the first
    block is always 0/N before convergence)."""
    total = len(claims)
    reached = sum(1 for c in claims if c.get("status") == "covered")
    return reached, total


# Heuristic: an `i >= <bound>` claim at a loop head is an unwind artefact.
EXIT_RE = re.compile(r"\bi\s*>=\s*\w+")


def load_claims(json_path: Path) -> list[dict]:
    """Return the `claims` array from cov-report.json, or [] on any error."""
    if not json_path.exists():
        return []
    try:
        data = json.loads(json_path.read_text())
    except json.JSONDecodeError:
        return []
    return data.get("claims", [])


def count_exit_uncov(claims: list[dict]) -> int:
    """Count uncovered loop-exit-edge claims (artefact of finite --unwind)."""
    return sum(1 for c in claims
               if c.get("status") == "uncovered"
               and EXIT_RE.search(c.get("condition", "")))


def per_function_rollup(claims: list[dict]) -> dict[str, list[int]]:
    """Group claims by enclosing function: {fn: [reached, total]}."""
    counts: dict[str, list[int]] = defaultdict(lambda: [0, 0])
    for c in claims:
        fn = c.get("function", "<?>")
        counts[fn][1] += 1
        if c.get("status") == "covered":
            counts[fn][0] += 1
    return dict(counts)


def infer_flag_family(cmd_path: Path) -> str:
    """Classify the saved invocation into LANG / FUNC / UTILS_LANG / ?."""
    if not cmd_path.exists():
        return "?"
    cmd = cmd_path.read_text()
    if "--k-induction" in cmd:
        return "FUNC"
    if "--ub-shift-check" in cmd and "--unsigned-overflow" not in cmd:
        return "UTILS_LANG"
    return "LANG"


def parse_unwind(cmd_path: Path) -> str:
    """Extract the unwind/k-step bound from the saved invocation."""
    if not cmd_path.exists():
        return "?"
    cmd = cmd_path.read_text()
    m = re.search(r"--unwind\s+(\d+)", cmd)
    if m:
        return m.group(1)
    if "--k-induction" in cmd:
        m = re.search(r"--max-k-step\s+(\d+)", cmd)
        return f"k≤{m.group(1)}" if m else "k-ind"
    return "?"


def collect_rows(cov_dir: Path) -> list[dict]:  # pylint: disable=too-many-locals
    """Walk results/cov/ and build one row dict per (target, phase).

    TARGETS produces both p1 and p2 rows.
    TARGETS_P1_ONLY produces a p1 row plus an n/a marker for p2 so the
    Markdown table maintains its per-module pair shape; the n/a row is
    elided from per-function rollups and from the p1→p2 delta table.
    """
    rows = []
    for target in TARGETS:
        for phase in PHASES:
            rows.append(_row_for(cov_dir, target, phase))
    for target in TARGETS_P1_ONLY:
        rows.append(_row_for(cov_dir, target, "p1"))
        rows.append({
            "target": target, "phase": "p2", "metric": "n/a",
            "reached": 0, "total": 0, "ratio": 0.0,
            "effective_ratio": 0.0, "unwind": "—",
            "flags": "—", "exit_uncov": 0, "claims": [],
            "p1_only": True,
        })
    return rows


def _row_for(cov_dir: Path, target: str, phase: str) -> dict:
    """Build a single (target, phase) row from disk artefacts."""
    jsn = cov_dir / f"{target}_{phase}.json"
    metric_file = cov_dir / f"{target}_{phase}.metric"
    cmd_file = cov_dir / f"{target}_{phase}.cmd"
    metric = (metric_file.read_text().strip()
              if metric_file.exists() else "?")
    claims = load_claims(jsn)
    reached, total = counts_from_claims(claims)
    exit_uncov = count_exit_uncov(claims)
    denom = max(1, total - exit_uncov)
    effective = reached / denom if total > 0 else 0.0
    ratio = reached / total if total > 0 else 0.0
    return {
        "target": target,
        "phase": phase,
        "metric": metric,
        "reached": reached,
        "total": total,
        "ratio": ratio,
        "effective_ratio": effective,
        "unwind": parse_unwind(cmd_file),
        "flags": infer_flag_family(cmd_file),
        "exit_uncov": exit_uncov,
        "claims": claims,
    }


def emit_tsv(rows: list[dict]) -> str:
    """Render rows as a tab-separated table with a header line."""
    out = ["\t".join([
        "target", "phase", "metric", "reached", "total",
        "ratio", "eff_ratio", "unwind", "flags", "exit_uncov"])]
    for r in rows:
        ratio = f"{r['ratio']:.3f}" if r["metric"] != "error" else "ERROR"
        eff = (f"{r['effective_ratio']:.3f}"
               if r["metric"] != "error" else "ERROR")
        out.append("\t".join([
            r["target"], r["phase"], r["metric"],
            str(r["reached"]), str(r["total"]),
            ratio, eff,
            r["unwind"], r["flags"], str(r["exit_uncov"])]))
    return "\n".join(out) + "\n"


def emit_markdown(rows: list[dict]) -> str:  # pylint: disable=too-many-locals
    """Render rows as a Markdown table + p1→p2 deltas + per-fn rollup."""
    out = ["## Phase 4 — Coverage summary",
           "",
           "| target | phase | metric | reached | total | ratio | eff. | "
           "unwind | flags | exit_uncov |",
           "|---|---|---|---:|---:|---:|---:|---:|---|---:|"]
    by_target: dict[str, dict[str, dict]] = defaultdict(dict)
    for r in rows:
        out.append(
            f"| `{r['target']}` | {r['phase']} | {r['metric']} | "
            f"{r['reached']} | {r['total']} | {r['ratio']:.3f} | "
            f"{r['effective_ratio']:.3f} | {r['unwind']} | {r['flags']} | "
            f"{r['exit_uncov']} |")
        by_target[r["target"]][r["phase"]] = r

    out.append("")
    out.append("### p1 → p2 deltas (|Δratio| ≥ 0.30 noted)")
    out.append("")
    out.append("| target | ratio_p1 | ratio_p2 | Δratio | note |")
    out.append("|---|---:|---:|---:|---|")
    for target in TARGETS:
        p1 = by_target.get(target, {}).get("p1")
        p2 = by_target.get(target, {}).get("p2")
        r1 = p1["ratio"] if p1 else 0.0
        r2 = p2["ratio"] if p2 else 0.0
        delta = r2 - r1
        note = ""
        if abs(delta) >= 0.30:
            note = ("k-induction collapsed paths"
                    if delta < 0 else "p2 explored more paths")
        out.append(f"| `{target}` | {r1:.3f} | {r2:.3f} | {delta:+.3f} | "
                   f"{note} |")
    if TARGETS_P1_ONLY:
        out.append("")
        out.append(f"_p1-only modules ({', '.join(f'`{t}`' for t in TARGETS_P1_ONLY)}) "
                   "are omitted from the Δ table — no p2 target exists. See "
                   "Phase 4 commentary for the per-module rationale._")

    out.append("")
    out.append("### Per-function rollup (top uncovered)")
    out.append("")
    for r in rows:
        if r.get("p1_only"):  # n/a placeholder row
            continue
        roll = per_function_rollup(r["claims"])
        if not roll:
            continue
        worst = sorted(((fn, reached, total)
                        for fn, (reached, total) in roll.items()
                        if total > 0 and reached < total),
                       key=lambda x: (x[1] / x[2] if x[2] else 1.0))
        if not worst:
            continue
        out.append(f"- `{r['target']}` ({r['phase']}, {r['metric']}):")
        for fn, reached, total in worst[:3]:
            out.append(f"    - `{fn}` — {reached}/{total} "
                       f"({reached/total:.0%})")
    return "\n".join(out) + "\n"


def main() -> int:
    """CLI entry: TSV by default, --markdown for the rendered table."""
    ap = argparse.ArgumentParser()
    ap.add_argument("cov_dir", type=Path)
    ap.add_argument("--markdown", action="store_true")
    args = ap.parse_args()
    if not args.cov_dir.is_dir():
        print(f"error: not a directory: {args.cov_dir}", file=sys.stderr)
        return 2
    rows = collect_rows(args.cov_dir)
    sys.stdout.write(emit_markdown(rows) if args.markdown else emit_tsv(rows))
    return 0


if __name__ == "__main__":
    sys.exit(main())
