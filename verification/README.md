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
| `mctp_dispatch` | F-1 reachability: `Validator::validate()` + production `on_set_endpoint_id()` | — | — | ✅ CEX: `iface_val=2`, `valid=true`, OOB at `cur_eid.at(2)` (275 VCC) |
| `mctp_validator` | `corepdk/.../app/pdk-mctp-app-validator.cpp` | ✅ 119 VCC | ✅ k=1 (full functional contract) | — |
| `nsm_type_2` | `src/nv/mctp/nsm_type_2.cpp` (`validatePcieLinkResetValue`) | ✅ | ✅ k=12 | — |
| `nsm_type3` | `src/nv/mctp/nsm_type_3.cpp` (`is_temp_sensor_available`, `is_power_sensor_available`, `is_voltage_sensor_available`) | ✅ 37 VCC | ✅ k=9 | — |
| `telemetry` | `src/nv/telemetry/utils.h` (`getTelemIdFromTempSensorId`, `getTelemIdFromPowerSensorId`, `buffer_to_uint32`) | ✅ 80 VCC | ✅ k=11 | — |
| `nsm_bitmask` | `src/nv/mctp/nsm_msg_bitmask.h` (`set_bit`/`unset_bit`/`get_bit`/`is_bit_set`) | ✅ 75 VCC | ✅ k=1 | ✅ CEX on `set_bit`/`unset_bit(arr8, pos≥64)` — F-5 |
| `nsm_type5_validate` | `src/nv/mctp/nsm_type_5.cpp` (five field-validator functions) | ✅ 14 VCC | ✅ k=1 | — |
| `spi_utils` | `src/nv/spi/utils.{h,cpp}` (buf_to_u16/u32, u16/u32_to_buf) | ✅ | ✅ k=9 | — |
| `i2c_crc8` | `src/nv/i2c/helper.cpp` (crc8) | ✅ | ✅ k=5 | — |
| `literals` | `src/nv/common/literals.h` (UDL truncation + shift) | ✅ | ✅ k=1 | ✅ CEX on `_bit(i≥64)` — F-4 |
| `fixed_point` | `src/nv/common/fixed_point.h` | ✅ | ✅ | — |
| `utils` | `src/nv/common/utils.h` (saturating add/sub/mul/align_to) | ✅ | ✅ | ⚠ (ESBMC strict unsigned-wrap demo, not a bug) |
| `ntc_table` | `src/nv/volt_mon/ntc_table.{h,cpp}` (binary search + linear interpolation on 166-entry NTC thermistor table; `ntc_resistance_to_temperature`, `ntc_voltage_to_temperature`, `ntc_adc_to_temperature`, `ntc_temperature_to_resistance`, `ntc_temp_to_adc_value`) | ✅ 227 VCC | ✅ k=9 | — |
| `pwr_smooth_params` | `src/nv/soc_pwr_smoothing/presets.{h,cpp}` (`OverrideParam::to_uint32`, `OverrideParam::from_uint32`, `is_valid_param_id`) | ✅ 72 VCC | ✅ k=1 | — |
| `fru_utils` | `src/nv/fru/fru.cpp` (`verify_checksum`, `decode_6bit_ascii`) | ✅ 76 VCC | ✅ k=9 | — |

## ESBMC issues filed

Every workaround in this tree maps to a specific filed issue. See `REPORT.md`
for the full table; brief view:

| Issue | State | Workaround |
|---|---|---|
| [#4180](https://github.com/esbmc/esbmc/issues/4180) | closed (split + fixed) | — |
| [#4182](https://github.com/esbmc/esbmc/issues/4182) | fixed by [#4187](https://github.com/esbmc/esbmc/pull/4187) | (removed) |
| [#4183](https://github.com/esbmc/esbmc/issues/4183) | fixed by [#4188](https://github.com/esbmc/esbmc/pull/4188) | (crash gone; `<array>` shim retained for #4190 reasons) |
| [#4190](https://github.com/esbmc/esbmc/issues/4190) | partial — [#4194](https://github.com/esbmc/esbmc/pull/4194) merged; [#4213](https://github.com/esbmc/esbmc/pull/4213) added `underlying_type_t`; aggregate-`<array>` still missing | thin `<span>` shim (avoids bundled-`<array>` collision); `<array>` shim retained for aggregate-init |
| [#4191](https://github.com/esbmc/esbmc/issues/4191) | fixed by [#4192](https://github.com/esbmc/esbmc/pull/4192); follow-up: pointer overload uses `reinterpret_cast` (drops const) | thin `<bit>` shim that uses C-cast for the pointer specialisation |
| [#4195](https://github.com/esbmc/esbmc/issues/4195) | fixed by [#4204](https://github.com/esbmc/esbmc/pull/4204) | (workaround removed) |
| [#4201](https://github.com/esbmc/esbmc/issues/4201) | resolved — [#4211](https://github.com/esbmc/esbmc/pull/4211) **merged** with a type-driven non-negativity predicate on `E1` (7 CORE regressions) | spi_utils harness uses production form directly (workaround removed) |
| [#4214](https://github.com/esbmc/esbmc/issues/4214) | `platforms::Control` default-construction crashes ESBMC (`clang_c_adjust_expr.cpp:158` assertion) | **fixed** by [#4215](https://github.com/esbmc/esbmc/pull/4215) (merged 2026-04-29) | (resolved; `ctrl{}` now constructs cleanly) |
| [#4216](https://github.com/esbmc/esbmc/issues/4216) | `switch(static_cast<enum>(packed_field))` + second field read in case body crashes SMT encoding (`mk_eq` bitvector width mismatch) | closed by [#4217](https://github.com/esbmc/esbmc/pull/4217); see #4232/#4234 for variants | — |
| [#4232](https://github.com/esbmc/esbmc/issues/4232) | `mk_eq`/`to_solver_smt_ast` crash persists after #4217: bitfield-base struct + aggregate-init + switch-case + member read | **fixed** by [#4233](https://github.com/esbmc/esbmc/pull/4233) | (removed) |
| [#4234](https://github.com/esbmc/esbmc/issues/4234) | `switch(static_cast<enum>(bit_cast member))` + `at()` in case body crashes `mk_eq` — fall-through label not normalised in `adjust_switch_case_ops` | **fixed** by [#4235](https://github.com/esbmc/esbmc/pull/4235) (merged 2026-05-01) | (workaround removed; production switch encodes correctly) |
| [#4237](https://github.com/esbmc/esbmc/issues/4237) | Value-init `struct Derived : class Base` via `{}` crashes `to_solver_smt_ast` (smt_ast.h:111) | **fixed** by [#4238](https://github.com/esbmc/esbmc/pull/4238) | (workaround removed; `ctrl{}` now constructs cleanly) |
| [#4240](https://github.com/esbmc/esbmc/issues/4240) | `--overflow-check` / `--ub-shift-check` generating false-positive signed-shl VCCs under `--std c++20` (C++20 [expr.shift]/2 makes signed left-shift fully defined; ESBMC was still applying pre-C++20 rules) | **fixed** by [#4241](https://github.com/esbmc/esbmc/pull/4241) | (no workaround needed; repro: `esbmc_bug_repros/signed_shift_result_overflow.cpp`) |

## Findings

- **F-1** (confirmed reachable):
  `Validator::validate()` guards `interface >= Interface::End` (18) but
  `RoutingTable::ec.cur_eid` has size `Interface::UsEnd` (2). Any interface
  in `[2, 17]` passes validation and then OOBs in `set_cur_eid()`. ESBMC
  proves this with a concrete counterexample (`iface_val=2`, `valid=true`,
  `cur_eid.at(2)` fails). Production builds with `-fno-exceptions`, so the
  OOB hits `abort()`. Fix: add an `interface >= UsEnd` guard to
  `set_cur_eid()`, or tighten `validate()` to reject `>= UsEnd`.
- **F-4** (latent UB, low severity):
  `operator""_bit(unsigned long long i)` in `src/nv/common/literals.h`
  computes `1ULL << i` with no guard — UB when `i >= 64` per
  `[expr.shift]/1`. All current call sites use compile-time constants ≤ 5, so
  no runtime exposure today. Fix: change `constexpr` → `consteval`.
- **F-5** (latent UB, low severity):
  `set_bit` and `unset_bit` on `std::array<uint8_t, NvMctpEventSupportedNum=8>`
  in `src/nv/mctp/nsm_msg_bitmask.h` call `bitmask.at(pos/8)` without a bounds
  guard. `get_bit` carries `if (byte_index < bitmask.size())` but the write
  operations do not. For `pos ≥ 64`, `byte_index ≥ 8` is OOB on a size-8 array
  (ESBMC CEX: `pos=248`, `byte_index=31`). All current call sites use constants
  4 and 5, so no runtime exposure today. Fix: add the same guard that `get_bit`
  already carries to both write operations.
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
make nsm_type3_func                            # ditto
make telemetry_func                            # ditto
make fixed_point_func utils_func               # ditto
make ntc_table_func                            # ditto
make pwr_smooth_params_func fru_utils_func     # ditto
make mctp_packet_neg mctp_router_neg utils_neg # negative tests (expect FAILED)
make mctp_dispatch                             # F-1 reachability proof (expect FAILED)
make nsm_bitmask_neg                           # F-5 OOB proof (expect FAILED)
```

Requires ESBMC on `$PATH` (current `master` recommended), or pass
`ESBMC=/path/to/esbmc make ...`.
