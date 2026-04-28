# ESBMC Verification of OpenSMA

Bounded model-checking harnesses for selected modules of NVIDIA OpenSMA, run
against [ESBMC](https://github.com/esbmc/esbmc) on macOS aarch64.

## Layout

```
verification/
├── Makefile                          # ESBMC invocations per target/profile
├── run.sh                            # thin wrapper: ./run.sh mctp_packet
├── harnesses/                        # *_harness.cpp per target (Phase 1 + Phase 2)
├── stubs/                            # verification-only header shims
│   ├── array                         # esbmc#4190 (aggregate-init divergence)
│   ├── span, bit                     # thin replacements over post-#4194/#4192 bundled
│   ├── pdk-cmn-flowcontrol.h         # drops upstream Ada/log dep
│   └── pdk/cmn/log/log.h             # no-op log shim
├── ctest/{f1,f2,f3}/                 # ESBMC --generate-ctest-testcase outputs
├── esbmc_bug_repros/                 # standalone repros for upstream ESBMC bugs
└── results/                          # esbmc logs (regenerated on `make`)
```

## Profiles

`Makefile` exposes two ESBMC profiles per target:

- `<target>` — language-level safety (default checks + `--memory-leak-check
  --overflow-check --unsigned-overflow-check --nan-check --unwind 4`).
- `<target>_func` — functional contracts via `--k-induction --interval-analysis
  --k-step 1 --max-k-step 6` (validator uses `--max-k-step 16` for its branchier
  state machine), with `-DESBMC_FUNCTIONAL=1` enabling extra `__ESBMC_assert`
  in the harness.

Negative tests (`<target>_neg`) deliberately drive the unsafe path and expect
ESBMC to produce a counterexample.

## Targets

| Target | Source | Phase 1 | Phase 2 | Negative |
|---|---|:-:|:-:|:-:|
| `mctp_packet` | `corepdk/.../app/pdk-mctp-app-packet.cpp` | ✅ | ✅ | ✅ |
| `mctp_router` | `corepdk/.../platforms/x86/pdk-mctp-platforms-router-plat.cpp` | ✅ | ✅ | ✅ |
| `mctp_validator` | `corepdk/.../app/pdk-mctp-app-validator.cpp` | ✅ 119 VCC | ✅ k=1 (full functional contract) | — |
| `nsm_type_2` | `src/nv/mctp/nsm_type_2.cpp` (`validatePcieLinkResetValue`) | ✅ | ✅ k=12 | — |
| `spi_utils` | `src/nv/spi/utils.{h,cpp}` (buf_to_u16/u32, u16/u32_to_buf) | ✅ | ✅ k=9 | — |
| `i2c_crc8` | `src/nv/i2c/helper.cpp` (crc8) | ✅ | ✅ k=5 | — |
| `literals` | `src/nv/common/literals.h` (UDL truncation + shift) | ✅ | ✅ k=1 | ✅ CEX on `_bit(i≥64)` |
| `fixed_point` | `src/nv/common/fixed_point.h` | ✅ | ✅ | — |
| `utils` | `src/nv/common/utils.h` (saturating add/sub/mul/align_to) | ✅ | ✅ | ⚠ (ESBMC strict unsigned-wrap demo, not a bug) |

## ESBMC issues filed

Every workaround in this tree maps to a specific filed issue. See `REPORT.md`
for the full table; brief view:

| Issue | State | Workaround |
|---|---|---|
| [#4180](https://github.com/esbmc/esbmc/issues/4180) | closed (split + fixed) | — |
| [#4182](https://github.com/esbmc/esbmc/issues/4182) | fixed by [#4187](https://github.com/esbmc/esbmc/pull/4187) | (removed) |
| [#4183](https://github.com/esbmc/esbmc/issues/4183) | fixed by [#4188](https://github.com/esbmc/esbmc/pull/4188) | (crash gone; `<array>` shim retained for #4190 reasons) |
| [#4190](https://github.com/esbmc/esbmc/issues/4190) | partial — [#4194](https://github.com/esbmc/esbmc/pull/4194) merged; aggregate-`<array>` and `underlying_type_t` still missing | thin `<span>` shim (avoids bundled-`<array>` collision); `<array>` shim retained; `utils.h` inlined for `underlying_type_t` |
| [#4191](https://github.com/esbmc/esbmc/issues/4191) | fixed by [#4192](https://github.com/esbmc/esbmc/pull/4192); follow-up: pointer overload uses `reinterpret_cast` (drops const) | thin `<bit>` shim that uses C-cast for the pointer specialisation |
| [#4195](https://github.com/esbmc/esbmc/issues/4195) | fixed by [#4204](https://github.com/esbmc/esbmc/pull/4204) | (workaround removed) |
| [#4201](https://github.com/esbmc/esbmc/issues/4201) | open — [#4203](https://github.com/esbmc/esbmc/pull/4203) reverted by [#4208](https://github.com/esbmc/esbmc/pull/4208) (skip too broad); refined replacement [#4211](https://github.com/esbmc/esbmc/pull/4211) open with a type-driven non-negativity predicate on `E1` and 8 CORE regressions | spi_utils harness parenthesises cast inside shift on mainline; drops once #4211 lands |

## Findings

- **F-1** (defensive observation, reachability not traced):
  `pdk::mctp::platforms::set_cur_eid()` writes to a 2-element array
  indexed by a wire-supplied `interface` without bounds-checking. Whether
  the dispatch path can deliver `interface >= UsEnd` is not verified here.
  Production builds with `-fno-exceptions`, so an OOB hit goes to
  `abort()`/UB, not an uncaught throw. Empirical SIGABRT confirmed on
  host via ESBMC's ctest gen (see `ctest/f1/`). Fix is one line (mirror
  `get_cur_eid`).
- **F-2** *retracted*: initially claimed overflow in `align_to`; on
  review, the unsigned wrap is mathematically benign (cancels exactly
  under the subsequent mask). ESBMC's `--unsigned-overflow-check` flagged
  defined behaviour, not a defect — confirmed empirically by ESBMC's ctest
  gen (see `ctest/f2/`).
- **F-3** *retracted*: initially claimed signed-shift overflow in
  `buf_to_u32`; under `-std=c++23` (production), `int(byte) << 24` is a
  defined wrap producing the correct `uint32_t` bit pattern. ESBMC proved
  the production form byte-equivalent to the parenthesised form across
  the input space; same shape as F-2.

See `REPORT.md` for full discussion.

## Reproducing

```sh
cd verification
make all                                       # Phase 1 across every target
make mctp_packet_func mctp_router_func \
     mctp_validator_func                       # Phase 2 (k-induction)
make fixed_point_func utils_func               # ditto
make mctp_packet_neg mctp_router_neg utils_neg # negative tests (expect FAILED)
```

Requires ESBMC on `$PATH` (current `master` recommended), or pass
`ESBMC=/path/to/esbmc make ...`. The validator target requires that
upstream's `pdk-mctp-app-validator.cpp` is sed-patched at build time;
`results/pdk-mctp-app-validator.patched.cpp` is the generated copy.
