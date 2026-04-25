# ESBMC Verification of OpenSMA

Bounded model-checking harnesses for selected modules of NVIDIA OpenSMA, run
against [ESBMC](https://github.com/esbmc/esbmc) 8.2.0 (verified) on macOS aarch64.

## Layout

```
verification/
├── Makefile                 # ESBMC invocations per target/profile
├── run.sh                   # thin wrapper: ./run.sh mctp_packet
├── harnesses/               # one *_harness.cpp per target (Phase 1 + Phase 2)
├── stubs/                   # verification-only header overlays + minimal STL shims
├── esbmc_bug_repros/        # standalone reproducers for ESBMC frontend bugs
└── results/                 # esbmc logs (latest run; regenerated on `make`)
```

## Profiles

`Makefile` exposes two ESBMC profiles per target:

- `<target>` — language-level safety (default checks + `--memory-leak-check
  --overflow-check --unsigned-overflow-check --nan-check --unwind 4`).
- `<target>_func` — functional contracts via `--k-induction --k-step 1
  --max-k-step 6`, with `-DESBMC_FUNCTIONAL=1` enabling extra `__ESBMC_assert`
  in the harness.

Negative tests (`<target>_neg`) deliberately drive the unsafe path and expect
ESBMC to produce a counterexample.

## Targets

| Target | Source | Phase 1 | Phase 2 | Negative |
|---|---|:-:|:-:|:-:|
| `mctp_packet` | `corepdk/.../app/pdk-mctp-app-packet.cpp` | ✅ | ✅ | ✅ |
| `mctp_router` | `corepdk/.../platforms/x86/pdk-mctp-platforms-router-plat.cpp` | ✅ | ✅ | ✅ |
| `fixed_point` | `src/nv/common/fixed_point.h` | ✅ | ✅ | — |
| `utils` | `src/nv/common/utils.h` (saturating add/sub/mul/align_to) | ✅ | ✅ | ⚠ (ESBMC strict unsigned-wrap demo, not a bug) |

## ESBMC frontend bugs filed

Three C++ frontend bugs were discovered while preparing harnesses; minimal
reproducers live under `esbmc_bug_repros/`. Each is worked around in `stubs/`
and the workaround sites are tagged `WORKAROUND esbmc#<n>`.

- [esbmc/esbmc#4180](https://github.com/esbmc/esbmc/issues/4180) —
  `<array>` instantiation crashes the converter; namespace-qualified
  `constexpr` initialiser crashes the converter.
- [esbmc/esbmc#4182](https://github.com/esbmc/esbmc/issues/4182) —
  `using ns::T;` for class or enum types triggers
  `Conversion of unsupported clang type: Using`.

## Findings

- **F-1** (defensive observation, reachability not traced):
  `pdk::mctp::platforms::set_cur_eid()` writes to a 2-element array
  indexed by a wire-supplied `interface` without bounds-checking. Whether
  the dispatch path can deliver `interface >= UsEnd` is not verified here.
  Production builds with `-fno-exceptions`, so an OOB hit goes to
  `abort()`/UB, not an uncaught throw. Fix is one line (mirror
  `get_cur_eid`).
- **F-2** *retracted*: initially claimed overflow in `align_to`; on
  review, the unsigned wrap is mathematically benign (cancels exactly
  under the subsequent mask). ESBMC's `--unsigned-overflow-check` flagged
  a defined behaviour, not a defect. The proposed parenthesisation is a
  readability change, not a correctness one.

See `REPORT.md` for full discussion.

## Reproducing

```sh
cd verification
make mctp_packet         # Phase 1
make mctp_packet_func    # Phase 2 (k-induction)
make mctp_packet_neg     # negative test (expect VERIFICATION FAILED)

make mctp_router  mctp_router_func  mctp_router_neg
make fixed_point  fixed_point_func
make utils        utils_func        utils_neg
```

Requires `ESBMC` 8.2.0 on `$PATH` or pass `ESBMC=/path/to/esbmc make ...`.
