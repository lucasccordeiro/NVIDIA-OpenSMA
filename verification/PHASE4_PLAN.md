# Phase 4 — Coverage of Verified OpenSMA Modules (DRAFT, refined)

**Status**: draft, pending approval
**Proposed branch**: `verify/phase4-coverage`
**Base**: `vr_sma` (after current `feat/verify-event-srcs-ack-handlers` merges)
**Reference ESBMC build**: `/Users/user/esbmc/build/src/esbmc/esbmc` (8.2.0, Bitwuzla 0.8.2) — same as Phases 1–3.

## Goal

Measure how thoroughly the existing harnesses exercise the modules whose
Phase 1 / Phase 2 proofs are already structural (Tier A–E in `REPORT.md`).
Phase 4 is a **harness-quality metric**: it answers *"are our nondet
harnesses actually exercising the program paths that matter?"*, not
*"is the code safe?"*. **Existing safety verdicts in `REPORT.md` are not
revisited or invalidated by Phase 4 outcomes.**

## Flag choice — corrected after a trial run

Earlier draft assumed `--path-coverage`. ESBMC has no such flag. The
relevant flags are:

| Flag | What it counts | Where useful |
|---|---|---|
| `--k-path-coverage [=N]` | k-path witnesses (PathCrawler-style; bounded prefixes through CFG; depth N defaults to `--unwind`, fallback 4) | Harnesses whose branching lives at the harness level (e.g. `switch (nondet)`). Strongest signal when present. |
| `--k-path-witness-depth D` | Cap on post-simplification witness expression depth (default 8). | Tuning knob. Increasing it does **not** rescue flat-CFG harnesses (trial: bumping to 20 left soc_sma_filter at 0 goals). |
| `--branch-function-coverage` | Branch edges + function entry points | Always defined; useful fallback for harnesses where k-path returns 0 goals. |
| `--cov-assume-asserts` | (modifier) Convert assertions to assumptions during coverage so asserts don't cut path constraints | **Required** for both metrics — without it, asserts truncate paths and the metric understates. |
| `--cov-report-json` | Emit per-claim JSON to `cov-report.json` in CWD | Required for the per-function rollup. |

**Decision**: per target, pick whichever metric ESBMC actually populates.

- Run k-path-coverage first; if `k-Path Witnesses > 0`, that target reports
  k-path.
- Otherwise fall back to `--branch-function-coverage`.
- The chosen metric is recorded in a `metric` column so the table is
  honest about what was measured.

This was validated on two trial targets:

| Trial target | `--k-path-coverage` outcome | `--branch-function-coverage` outcome |
|---|---|---|
| `i2c_crc8_harness.cpp` | 7 / 10 reached → **70 %** | (not run; k-path was sufficient) |
| `soc_sma_filter_harness.cpp` | 0 goals (`N/A`) — flat harness CFG, deep header inlining | 4 / 4 reached → **100 %** |

(See Appendix A for the exact commands.)

## What "VERIFICATION FAILED" means under coverage mode

`--k-path-coverage` and `--branch-function-coverage` activate
`--multi-property`. ESBMC then enumerates every coverage goal as a claim
and the run-end summary line is "VERIFICATION FAILED" whenever **any**
goal is unreached — even when the underlying program is safe. This is a
property of coverage mode, not a regression. Phase 4 must therefore:

- Treat the textual `VERIFICATION SUCCESSFUL/FAILED` line in coverage logs
  as *not* a safety signal. The Phase 1/2 logs in `results/` keep the
  safety verdicts.
- Parse the `[Coverage]` block (or the JSON) for the metric.
- Make this distinction explicit in the REPORT section so a reader who
  greps for "VERIFICATION FAILED" across `results/` does not panic.

## Non-goals

- Re-verifying safety. Phase 1/2 verdicts are unchanged.
- Coverage as a CI gate. Phase 4 emits an advisory report, not a pass/fail.
- Coverage of dead-code-only paths (e.g. F-10 in `ntc_table` where the
  bug path is unreachable in any current build). The module's reachable
  paths are still measured.
- Coverage of Tier F/G straight-line CEX harnesses (single OOB path by
  design — see exclusion list below).

## Scope — modules included

Selected on: production source compiled (Tier A–E) AND harness drives a
non-trivial control flow.

| # | Module | Harness | Notes |
|---|---|---|---|
| 1 | DCD event-bitmask write handlers | `nsm_dcd_event_handlers_harness.cpp` | Multi-handler dispatch; uses `--unwind 10` and a custom flag set |
| 2 | SoC SMA filter | `soc_sma_filter_harness.cpp` | Trialed: k-path 0, branch-fn 100 % / 4 — keep, low signal expected |
| 3 | Debug telemetry SMA | `debug_telemetry_sma_harness.cpp` | 256-sample SMA + system-level latency proof (F-13) |
| 4 | PCA9555 GPIO expander | `pca9555_harness.cpp` | Register-mode dispatch CFG — likely k-path-rich |
| 5 | NTC thermistor table | `ntc_table_harness.cpp` | 166-entry lookup; harness has nondet driver |
| 6 | MCTP validator state machine | `mctp_validator_harness.cpp` | Stateful CFG |
| 7 | NSM bitmask ops | `nsm_bitmask_harness.cpp` | set/unset/get/is_set dispatch |
| 8 | MCTP packet parser | `mctp_packet_harness.cpp` | Field-path branching |
| 9 | MCTP router | `mctp_router_harness.cpp` | Per-interface paths |
| 10 | NSM type-5 field validators | `nsm_type5_validate_harness.cpp` | Multi-validator dispatch |

**Excluded** (Tier F/G negative-test harnesses; single straight-line OOB/UB
path by design): `literals_negative`, `nsm_bitmask_negative`,
`mctp_packet_negative`, `mctp_router_negative`, `mctp_validator_negative`,
`utils_negative`, `nsm_type5_f{6,7,8}`, `nsm_type3_f10_neg`,
`nsm_event_source_f15`, `nsm_event_ack_f16`, `nsm_gpio_safety`,
`debug_telemetry_f13`, `pca9555_f14`, `spi_utils_f9`, `fru_f12`.

**Deferred to Phase 4.1** (simpler CFGs; will benefit less from coverage,
include if the in-scope batch surfaces useful signal): `fixed_point`,
`utils`, `literals`, `i2c_crc8`, `spi_utils`, `telemetry`, `fru_utils`,
`pwr_smooth_params`, `emc1812`, `tmp1075`, `tmp461`, `nsm_type_2`,
`nsm_type3`.

## Method

### Per-target Makefile rules

Two rules per target — `_cov_p1` (uses the target's existing Phase 1
flag profile) and `_cov_p2` (uses `FUNC_FLAGS`, i.e. k-induction).
Output goes to `$(RESULTS)/cov/`, never to `$(RESULTS)/` directly.
The Make rule is a thin wrapper; everything else lives in
`scripts/cov_run.sh` (see below).

```make
COV     := $(RESULTS)/cov
COV_RUN := ./scripts/cov_run.sh

soc_sma_filter_cov_p1:
	@mkdir -p $(COV)
	@$(COV_RUN) soc_sma_filter p1 $(ESBMC) \
	  $(LANG_FLAGS) -I$(STUBS) -I$(REPO)/src \
	  -- $(HARNESSES)/soc_sma_filter_harness.cpp

soc_sma_filter_cov_p2:
	@mkdir -p $(COV)
	@$(COV_RUN) soc_sma_filter p2 $(ESBMC) \
	  $(FUNC_FLAGS) -DESBMC_FUNCTIONAL=1 -I$(STUBS) -I$(REPO)/src \
	  -- $(HARNESSES)/soc_sma_filter_harness.cpp
```

`cov_run.sh <target> <phase> <esbmc> <flags…> -- <inputs…>` does the
full per-run choreography in one place:

1. Resolves any relative path in `<flags>`/`<inputs>` to absolute (so
   the `cd` into the per-target work-dir does not break input lookup).
2. Records the resolved invocation to `<target>_<phase>.cmd` so the
   aggregator can extract `--unwind` and the flag profile reliably
   (ESBMC's stdout doesn't echo argv).
3. Runs ESBMC with `--k-path-coverage --cov-assume-asserts
   --cov-report-json`. If the resulting log shows
   `k-Path Witnesses : 0`, re-runs with `--branch-function-coverage`
   instead (same flag profile, same work-dir).
4. Writes `<target>_<phase>.metric` with `k-path` / `branch-fn` /
   `error` (the last when no `[Coverage]` block was emitted, e.g. flag
   rejected, parse error — distinct from "0 reached").
5. Moves `cov-report.json` to `<target>_<phase>.json` and removes the
   work-dir.

Each target's existing flag profile and unwind bound are reused —
coverage runs the **same proof** as Phase 1/2, not a stripped-down
build.

The per-target work-dir + post-run `mv` dance is forced by ESBMC writing
`cov-report.json` to CWD; per-`(target, phase)` directories
(`work-<target>-p{1,2}`) keep parallel `make -j` runs collision-free.

For modules whose existing `_func` rule has its own flag tweaks (e.g.
`nsm_type_2_func` adds `--unwind 12`, `nsm_type3_func` adds custom
includes), the `_cov_p2` rule mirrors those tweaks verbatim. Per-target
overrides are encoded directly in each `_cov_p2` rule, not centralised.

### Aggregate target

```make
COV_TARGETS := nsm_dcd_event_handlers soc_sma_filter debug_telemetry_sma \
               pca9555 ntc_table mctp_validator nsm_bitmask mctp_packet \
               mctp_router nsm_type5_validate

cov_p1: $(addsuffix _cov_p1, $(COV_TARGETS))
cov_p2: $(addsuffix _cov_p2, $(COV_TARGETS))

cov: cov_p1 cov_p2
	@scripts/cov_aggregate.py $(COV) > $(COV)/cov_summary.tsv
	@scripts/cov_aggregate.py --markdown $(COV) \
	  > $(COV)/cov_summary.md
```

`cov_aggregate.py` reads every `<target>_<phase>.json` under
`results/cov/`, picks the metric per `(target, phase)` log, and emits
one TSV row per `(target, phase)` plus a rendered Markdown table.

## Results-table format

One row per `(target, phase)` (TSV; rendered as Markdown for
`REPORT.md § Phase 4`):

```
target                          phase metric    reached total ratio  unwind flags         exit_uncov
soc_sma_filter                  p1    branch-fn   4      4    1.000   4     LANG          0
soc_sma_filter                  p2    …           …      …    …       4     FUNC          …
debug_telemetry_sma             p1    k-path      …      …    …       4     LANG          …
debug_telemetry_sma             p2    k-path      …      …    …       4     FUNC          …
…                               …     …           …      …    …       …     …             …
nsm_dcd_event_handlers          p1    k-path      …      …   10      LANG (custom) …
nsm_dcd_event_handlers          p2    k-path      …      …   10      FUNC (custom) …
```

Columns:

- `target` — Make target stem (no `_cov_p{1,2}` suffix).
- `phase` — `p1` (Phase 1, `LANG_FLAGS`) or `p2` (Phase 2, `FUNC_FLAGS`).
- `metric` — `k-path` or `branch-fn` (whichever ESBMC populated for
  this `(target, phase)`).
- `reached` / `total` — directly from the JSON; `ratio = reached/total`.
- `unwind` — the harness's existing `--unwind` (unchanged from
  Phase 1/2).
- `flags` — `LANG` / `UTILS_LANG` / `FUNC` / `custom` indicating the
  flag profile family.
- `exit_uncov` — count of uncovered claims of the form `i >= <bound>`
  at loop heads, identified by `cov_aggregate.py`. Unwind artifacts;
  excluded from the rubric.

Two derived views are also emitted:

- **Per-function rollup** (function name → reached/total per `(target,
  phase)`), as a sibling block beneath each module's pair of rows.
- **p1-vs-p2 delta** column in the Markdown view: `Δratio =
  ratio_p2 - ratio_p1`. A large negative Δ indicates k-induction
  collapsed the path set on that module — informative, not a problem.

## Interpretation rubric

Applied **per row** (each `(target, phase)` independently). For each row,
compute `effective_ratio = reached / max(1, total - exit_uncov)`:

- `effective_ratio ≥ 0.90` — harness is path-rich; no action.
- `0.70 ≤ effective_ratio < 0.90` — record; flag if uncovered claims
  cross the module's main control split. Otherwise no action.
- `effective_ratio < 0.70` — open a follow-up issue: harness likely
  over-`assume`-pruned; review which paths are dead.
- `total ≤ 3` (after excluding exit branches) — harness is under-driven
  *for this phase*; follow up regardless of ratio.
- `metric == k-path` and `total == 0` — neutral; signals the harness's
  branching lives in deeply inlined code beyond the witness-depth cap.
  The branch-function fallback row supersedes.
- `Δratio = ratio_p2 - ratio_p1 < -0.30` — informational, not a defect:
  k-induction collapsed the path set on this module. Note in REPORT
  but no follow-up needed.

Thresholds are advisory, not gates. A target requiring follow-up at
**either** phase is enough to open one issue (do not double-file).

## Deliverables

1. `verification/Makefile` — `<target>_cov_p1` and `<target>_cov_p2`
   rules per in-scope module, plus aggregates `cov_p1`, `cov_p2`, `cov`.
2. `verification/scripts/cov_run.sh` — full per-run wrapper:
   path-resolution, ESBMC invocation, k-path → branch-fn fallback,
   `.cmd` / `.metric` recording, JSON rename, work-dir cleanup.
   Parameterised by `(target, phase)`.
3. `verification/scripts/cov_aggregate.py` — JSON → TSV/Markdown,
   per-function rollup, p1-vs-p2 delta.
4. `verification/results/cov/<target>_p{1,2}.{log,json}` — raw per-run
   output (Phase 1/2 safety logs in `results/` are untouched).
5. `verification/results/cov/cov_summary.{tsv,md}` — aggregate with
   one row per `(target, phase)`.
6. `verification/REPORT.md § Phase 4` — short section with the rendered
   table, a one-paragraph note per outlier flagged by the rubric, the
   p1-vs-p2 delta observations, and an explicit reminder that
   "VERIFICATION FAILED" inside `results/cov/*.log` is not a safety
   regression.
7. No changes to existing Phase 1/2 targets, harnesses, `results/`
   logs, or verdicts.

## Acceptance criteria

- All twenty `<target>_cov_p{1,2}` targets (10 modules × 2 phases)
  terminate within the project's 5-minute regression budget on the
  reference ESBMC build. If any individual run exceeds the budget,
  narrow the harness's nondet inputs (do not raise `--unwind`) and
  document the constraint in REPORT.
- Each `(target, phase)` row produces either a k-path metric
  (`total > 0`) or a branch-function fallback (always defined). Every
  TSV row names its `metric` and `phase` explicitly.
- `results/cov/cov_summary.{tsv,md}` are committed and reproducible
  from a clean checkout via `make cov`. `make cov_p1` and
  `make cov_p2` are independently runnable.
- For every `(target, phase)` flagged by the rubric, REPORT names the
  uncovered claims (file:line + condition) and either (a) opens a
  follow-up to strengthen the harness, or (b) documents why the path
  is unreachable by construction.
- p1-vs-p2 deltas with `|Δratio| ≥ 0.30` are noted in REPORT (one line
  each) so the comparison is explicit, not buried in the table.
- Existing Phase 1, Phase 2, and negative-test verdicts in `REPORT.md`
  and the contents of `verification/results/` (the safety logs) are
  byte-for-byte unchanged.

## Risks and mitigations

- **k-path returns N/A on flat harnesses** (validated on
  `soc_sma_filter`). Mitigation: branch-function fallback, encoded in
  `cov_run.sh`.
- **`VERIFICATION FAILED` panic** on a casual `grep`. Mitigation:
  segregate coverage logs under `results/` with the `_cov.log` suffix and
  state the convention prominently in the REPORT section.
- **`--cov-report-json` path collision under parallel make**. Mitigation:
  the per-target rule `cd`s into a per-target subdir and `mv`s the JSON
  to a uniquely named output before any parallel job touches CWD.
- **Goal-count blowup on `pca9555`** (1292 VCC in Phase 1, large CFG).
  Mitigation: default `--k-path-max-goals=10000` should hold; if it
  doesn't, narrow the harness's nondet inputs to the same subset Phase 1
  constrains and document the constraint. Do **not** raise `--unwind` to
  chase coverage.
- **ESBMC version drift**. Mitigation: pin the commit in REPORT § Phase 4
  (matching Phases 1–3); coverage flag set was confirmed against
  ESBMC 8.2.0 in the trial.
- **Citation hygiene**: ESBMC's `--k-path-coverage` help string
  references "Williams et al., EDCC 2005 (PathCrawler-style)". Per the
  global citation rule, any user-visible citation in REPORT.md must be
  verified via `citation-verifier` before shipping. The plan's REPORT
  section should cite the ESBMC `--help` string verbatim, not paraphrase
  the underlying paper, until the reference is verified.

## Decisions (frozen 2026-05-10)

1. **Profile choice**: measure coverage on **both** Phase 1 (`LANG_FLAGS`)
   and Phase 2 (`FUNC_FLAGS`). Each target produces two coverage runs.
   The TSV/Markdown table gains a `phase` column (`p1` / `p2`) so each
   in-scope module contributes two rows. Comparing p1 vs p2 surfaces
   whether k-induction collapses the path set on that module.
2. **Results location**: all coverage artefacts live under
   `verification/results/cov/`, segregated from Phase 1/2 safety logs in
   `verification/results/`. This prevents `grep VERIFICATION
   results/` from conflating coverage-mode failures with safety
   regressions.
3. **Scope**: accept the 10-module shortlist as-is. The deferred set
   stays deferred to a possible Phase 4.1.

---

## Appendix A — Trial commands and outputs (run 2026-05-10 against ESBMC 8.2.0)

```bash
# k-path on i2c_crc8 (k-path-rich harness)
$ esbmc --std c++20 --memory-leak-check --overflow-check \
        --unsigned-overflow-check --nan-check --unwind 4 \
        --k-path-coverage --cov-report-json \
        -I./stubs -I../src ./harnesses/i2c_crc8_harness.cpp
… [Coverage] k-Path Witnesses : 10 ; Reached : 7 ; k-Path Coverage: 70%

# k-path on soc_sma_filter (flat harness CFG)
$ esbmc … --k-path-coverage --cov-assume-asserts --cov-report-json …
… [Coverage] k-Path Witnesses : 0 ; Reached : 0 ; k-Path Coverage: N/A

# k-path-witness-depth bumped to 20 — still N/A on soc_sma_filter
… [Coverage] k-Path Witnesses : 0 ; Reached : 0 ; k-Path Coverage: N/A

# branch-function fallback on soc_sma_filter
$ esbmc … --branch-function-coverage --cov-assume-asserts --cov-report-json …
… [Coverage] Function Entry Points & Branches : 4 ; Reached : 4 ; Branch Coverage: 100%
```

JSON record schema (one per claim):

```json
{ "file": "...", "function": "...", "line": N, "column": N,
  "condition": "...", "status": "covered" | "uncovered" }
```
