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
│   ├── algorithm                     # esbmc#4251 (std::clamp missing from bundled <algorithm>)
│   ├── power_manager_config.h        # NV_IPC_CONFIG_H substitute for F-13 system harness
│   ├── sys/{adc,dac,gpio}/           # hardware stubs for F-13 system harness
│   ├── pdk-cmn-flowcontrol.h         # drops upstream Ada/log dep
│   └── pdk/cmn/log/log.h             # no-op log shim
├── ctest/{f1,f2,f3}/                 # ESBMC --generate-ctest-testcase outputs
├── esbmc_bug_repros/                 # standalone repros for upstream ESBMC bugs
└── results/                          # esbmc logs (regenerated on `make`)
```

## Profiles

`Makefile` exposes two ESBMC profiles per target:

- `<target>` — language-level safety (`--memory-leak-check --overflow-check
  --unsigned-overflow-check --nan-check`). Most targets use the default
  `--unwind 4`; targets whose internal arrays exceed four elements carry a
  per-target override (see Makefile comments: `nsm_type_2` → 12,
  `nsm_type3` → 9, `telemetry` → 11, `spi_utils` → 9, `i2c_crc8` → 257,
  `fru_utils` → 9, `ntc_table` → 9). Loop exhaustion is formally checked via
  ESBMC's default unwinding assertions — the harness fails if any loop
  genuinely needs more than the stated bound.
- `<target>_func` — functional contracts via `--k-induction --interval-analysis
  --k-step 1 --max-k-step 6` (validator uses `--max-k-step 16` for its branchier
  state machine), with `-DESBMC_FUNCTIONAL=1` enabling extra `__ESBMC_assert`
  in the harness.
- `<target>_volatile` — Phase 1 flags plus `--volatile-check`, run as a
  separate phase so volatile findings do not block the Phase 1 CI gate.
  Run all with `make volatile`.

Negative tests (`<target>_neg`) deliberately drive the unsafe path and expect
ESBMC to produce a counterexample.

## Targets

| Target | Source | Phase 1 | Phase 2 | Volatile | Negative |
|---|---|:-:|:-:|:-:|:-:|
| `mctp_packet` | `corepdk/.../app/pdk-mctp-app-packet.cpp` | ✅ | ✅ | ✅ | ✅ |
| `mctp_router` | `corepdk/.../platforms/x86/pdk-mctp-platforms-router-plat.cpp` | ✅ | ✅ | ✅ | ✅ |
| `mctp_dispatch` | F-1 reachability: `Validator::validate()` + production `on_set_endpoint_id()` | — | — | — | ✅ CEX: `iface_val=2`, `valid=true`, OOB at `cur_eid.at(2)` (275 VCC) |
| `mctp_validator` | `corepdk/.../app/pdk-mctp-app-validator.cpp` | ✅ 119 VCC | ✅ k=1 (full functional contract) | ✅ | — |
| `nsm_type_2` | `src/nv/mctp/nsm_type_2.cpp` (`validatePcieLinkResetValue`) | ✅ | ✅ k=12 | ✅ | — |
| `nsm_type3` | `src/nv/mctp/nsm_type_3.cpp` (`is_temp_sensor_available`, `is_power_sensor_available`, `is_voltage_sensor_available`) | ✅ 37 VCC | ✅ k=9 | ✅ | ✅ **F-10** CEX: `threshold=254` → Success returned for out-of-range temperature |
| `telemetry` | `src/nv/telemetry/utils.h` (`getTelemIdFromTempSensorId`, `getTelemIdFromPowerSensorId`, `buffer_to_uint32`) | ✅ 80 VCC | ✅ k=11 | ✅ | — |
| `nsm_bitmask` | `src/nv/mctp/nsm_msg_bitmask.h` (`set_bit`/`unset_bit`/`get_bit`/`is_bit_set`) | ✅ 75 VCC | ✅ k=1 | ✅ | ✅ CEX on `set_bit`/`unset_bit(arr8, pos≥64)` — F-5 |
| `nsm_type5_validate` | `src/nv/mctp/nsm_type_5.cpp` (five field-validator functions) | ✅ 14 VCC | ✅ k=1 | ✅ | ✅ **F-6** CEX: `mode=0xFF` stored; **F-7** CEX: dirty `portRecoveryResp` on validation failure; **F-8** CEX: `gpio_ei_entries[16]` OOB |
| `spi_utils` | `src/nv/spi/utils.{h,cpp}` (buf_to_u16/u32, u16/u32_to_buf) | ✅ | ✅ k=9 | ✅ | — |
| `i2c_crc8` | `src/nv/i2c/helper.cpp` (crc8) | ✅ | ✅ k=5 | ✅ | — |
| `literals` | `src/nv/common/literals.h` (UDL truncation + shift) | ✅ | ✅ k=1 | ✅ | ✅ CEX on `_bit(i≥64)` — F-4 |
| `fixed_point` | `src/nv/common/fixed_point.h` | ✅ | ✅ | ✅ | — |
| `utils` | `src/nv/common/utils.h` (saturating add/sub/mul/align_to) | ✅ | ✅ | ✅ | ⚠ (ESBMC strict unsigned-wrap demo, not a bug) |
| `ntc_table` | `src/nv/volt_mon/ntc_table.{h,cpp}` (binary search + linear interpolation on 166-entry NTC thermistor table; `ntc_resistance_to_temperature`, `ntc_voltage_to_temperature`, `ntc_adc_to_temperature`, `ntc_temperature_to_resistance`, `ntc_temp_to_adc_value`) | ✅ 227 VCC | ✅ k=9 | ✅ | — |
| `pwr_smooth_params` | `src/nv/soc_pwr_smoothing/presets.{h,cpp}` (`OverrideParam::to_uint32`, `OverrideParam::from_uint32`, `is_valid_param_id`) | ✅ 72 VCC | ✅ k=1 | ✅ | — |
| `fru_utils` | `src/nv/fru/fru.cpp` (`verify_checksum`, `decode_6bit_ascii`) | ✅ 76 VCC | ✅ k=9 | ✅ | — |
| `soc_sma_filter` | `src/nv/soc_pwr_smoothing/soc_sma_filter_ch.h` (`SocSmaFilterCh::evaluate` — 4-sample sliding-window SMA over SFXP22_10) | ✅ 504 VCC | ✅ k=1 | ✅ | — |
| `debug_telemetry_sma` | `src/nv/soc_pwr_smoothing/debug_telemetry_sma_ch.h` (`DebugTelemetrySmaCh::evaluate` — 256-sample SMA; uint8_t buffer; percent ∈ [0%, 150%]) | ✅ 261 VCC | ✅ k=1 | ✅ | ✅ **F-13** CEX: `percent=-1024` → `stored=255` (negative wrap) |
| `debug_telemetry_f13_system` | F-13 reachability: `PowerManager::run_iteration()` with nondet ADC + GPIO (production code) | — | — | — | ✅ SUCCESSFUL (2081 VCC): upstream `std::clamp` prevents negative inputs from reaching `DebugTelemetrySmaCh` — **F-13 latent defect** |
| `pca9555` | `src/nv/emulation/pca9555.{h,cpp}` (`Pca9555` — 16-bit I2C GPIO expander emulator; direction/input/output/inversion registers + interrupt logic) | ✅ 1292 VCC | ✅ k=2 | ✅ | ✅ SUCCESSFUL (405 VCC, production `pca9555.cpp`) — **F-14 retracted** (bare `return` and `break` observably equivalent) |
| `emc1812` | `src/nv/i2c/emc1812.{h,cpp}` (`Emc1812` — EMC1812 temp sensor driver; `int8_t↔uint8_t` threshold cast round-trip) | ✅ 52 VCC | ✅ k=1 | ✅ | — |
| `tmp1075` | `src/nv/i2c/tmp1075.{h,cpp}` (`Tmp1075` — TMP1075 sensor driver; 12-bit temperature encoding: `int8_t → <<4 → uint16_t → >>4 → int8_t` round-trip) | ✅ 33 VCC | ✅ k=1 | ✅ | — |
| `tmp461` | `src/nv/i2c/tmp461.{h,cpp}` (`Tmp461` / NCT72 — sensor driver; `int8_t↔uint8_t` threshold cast round-trip for four set/get pairs) | ✅ 57 VCC | ✅ k=1 | ✅ | — |
| `nsm_event_source_f15_neg` | `src/nv/mctp/nsm.cpp:761` — `is_event_source_enable` reads `type0/6_event_enable_bitmask.at(event_id/8)` without bounds guard | — | — | — | ✅ FAILED — CEX: `event_id=248`, `ByteIndex=31`, OOB on size-8 array — **F-15** |
| `nsm_gpio_safety` | `src/nv/mctp/nsm.cpp:3389,3482` — `on_dcd_get_gpio` / `on_dcd_set_gpio` structural safety; `GpioNum=66` (p3957_cxx) | — | — | — | ✅ SUCCESSFUL (536 VCC) — guard sufficient, no defect |

## ESBMC issues filed

Every workaround in this tree maps to a specific filed issue. See `REPORT.md`
for the full table; brief view:

| Issue | State | Workaround |
|---|---|---|
| [#4180](https://github.com/esbmc/esbmc/issues/4180) | closed (split + fixed) | — |
| [#4182](https://github.com/esbmc/esbmc/issues/4182) | fixed by [#4187](https://github.com/esbmc/esbmc/pull/4187) | (removed) |
| [#4183](https://github.com/esbmc/esbmc/issues/4183) | fixed by [#4188](https://github.com/esbmc/esbmc/pull/4188) | (removed) |
| [#4190](https://github.com/esbmc/esbmc/issues/4190) | fully fixed — [#4194](https://github.com/esbmc/esbmc/pull/4194) + [#4213](https://github.com/esbmc/esbmc/pull/4213) + [#4244](https://github.com/esbmc/esbmc/pull/4244) (`<bit>`, `<span>`, `<type_traits>`, `<array>` aggregate) | (`stubs/array` removed) |
| [#4191](https://github.com/esbmc/esbmc/issues/4191) | fixed by [#4192](https://github.com/esbmc/esbmc/pull/4192); const-pointer follow-up tracked as #4247 | (see #4247) |
| [#4195](https://github.com/esbmc/esbmc/issues/4195) | fixed by [#4204](https://github.com/esbmc/esbmc/pull/4204) | (workaround removed) |
| [#4201](https://github.com/esbmc/esbmc/issues/4201) | resolved — [#4211](https://github.com/esbmc/esbmc/pull/4211) **merged** with a type-driven non-negativity predicate on `E1` (7 CORE regressions) | spi_utils harness uses production form directly (workaround removed) |
| [#4214](https://github.com/esbmc/esbmc/issues/4214) | `platforms::Control` default-construction crashes ESBMC (`clang_c_adjust_expr.cpp:158` assertion) | **fixed** by [#4215](https://github.com/esbmc/esbmc/pull/4215) (merged 2026-04-29) | (resolved; `ctrl{}` now constructs cleanly) |
| [#4216](https://github.com/esbmc/esbmc/issues/4216) | `switch(static_cast<enum>(packed_field))` + second field read in case body crashes SMT encoding (`mk_eq` bitvector width mismatch) | closed by [#4217](https://github.com/esbmc/esbmc/pull/4217); see #4232/#4234 for variants | — |
| [#4232](https://github.com/esbmc/esbmc/issues/4232) | `mk_eq`/`to_solver_smt_ast` crash persists after #4217: bitfield-base struct + aggregate-init + switch-case + member read | **fixed** by [#4233](https://github.com/esbmc/esbmc/pull/4233) | (removed) |
| [#4234](https://github.com/esbmc/esbmc/issues/4234) | `switch(static_cast<enum>(bit_cast member))` + `at()` in case body crashes `mk_eq` — fall-through label not normalised in `adjust_switch_case_ops` | **fixed** by [#4235](https://github.com/esbmc/esbmc/pull/4235) (merged 2026-05-01) | (workaround removed; production switch encodes correctly) |
| [#4237](https://github.com/esbmc/esbmc/issues/4237) | Value-init `struct Derived : class Base` via `{}` crashes `to_solver_smt_ast` (smt_ast.h:111) | **fixed** by [#4238](https://github.com/esbmc/esbmc/pull/4238) | (workaround removed; `ctrl{}` now constructs cleanly) |
| [#4240](https://github.com/esbmc/esbmc/issues/4240) | `--overflow-check` / `--ub-shift-check` generating false-positive signed-shl VCCs under `--std c++20` (C++20 [expr.shift]/2 makes signed left-shift fully defined; ESBMC was still applying pre-C++20 rules) | **fixed** by [#4241](https://github.com/esbmc/esbmc/pull/4241) | (no workaround needed; repro: `esbmc_bug_repros/signed_shift_result_overflow.cpp`) |
| [#4243](https://github.com/esbmc/esbmc/issues/4243) | bundled `<array>` value-init (`{}`) does not zero-initialise `elems` — elements are nondet; false-positive overflow VCCs on SMA filter accumulators | **fixed** by [#4244](https://github.com/esbmc/esbmc/pull/4244) (merged 2026-05-02) | (`stubs/array` removed) |
| [#4247](https://github.com/esbmc/esbmc/issues/4247) | bundled `<bit>` pointer-to-pointer `bit_cast` overload uses `reinterpret_cast`, rejecting const `From` (residual gap after #4191/#4192) | **fixed** by [#4250](https://github.com/esbmc/esbmc/pull/4250) (merged 2026-05-02) | (`stubs/bit` removed) |
| [#4248](https://github.com/esbmc/esbmc/issues/4248) | bundled `<span>` does not transitively include `<bit>`; production code relies on that transitive include for `std::bit_cast` | **fixed** by [#4249](https://github.com/esbmc/esbmc/pull/4249) (merged 2026-05-02) | (`stubs/span` removed) |
| [#4251](https://github.com/esbmc/esbmc/issues/4251) | bundled `<algorithm>` lacks `std::clamp` (C++17/20); also `const T&` shim return loses materialised value in GOTO IR | open | `stubs/algorithm` shim provides `std::clamp` returning `T` by value |
| [#2789](https://github.com/esbmc/esbmc/issues/2789) | negative shift distance (`x << y`, `y < 0`) not flagged under `--overflow-check`; only caught by `--ub-shift-check` | **fixed** by [#4242](https://github.com/esbmc/esbmc/pull/4242) (merged 2026-05-02) | — |

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
  (ESBMC CEX: `pos=248`, `byte_index=31`). Exhaustive call-graph trace: all 3
  production call sites use compile-time enum constants (≤ 5); no packet handler
  passes a runtime value to this overload — confirmed latent, no current
  packet-driven path. Fix: add the same guard that `get_bit` already carries to
  both write operations.
- **F-15** (latent OOB, low severity):
  `is_event_source_enable(NsmMsgType, uint8_t event_id)` in `nsm.cpp:761`
  computes `ByteIndex = event_id/8` and calls `type0/6_event_enable_bitmask.at(ByteIndex)`
  without a bounds check. Same asymmetric-guard pattern as F-5 but on the read
  path. For `event_id ≥ 64`, `ByteIndex ≥ 8` is OOB on the size-8 array
  (ESBMC CEX: `event_id=248`, `ByteIndex=31`). Current call sites use
  IPC-internal event IDs bounded well below 64. Fix: add
  `if (ByteIndex >= bitmask.size()) return false;` before the `.at()` calls.
- **F-6** (confirmed): `on_dev_cfg_set_errorInjectionMode` stores an unchecked
  `mode` byte — any value passes; no enum validation. ESBMC CEX: `mode=0xFF`
  stored unguarded.
- **F-7** (confirmed): `on_dev_cfg_set_portRecoveryErrorInjection` writes to
  `portRecoveryResp` before validating the payload — no rollback on failure.
  ESBMC CEX: dirty response field left in output buffer on invalid input.
- **F-8** (latent OOB): `validateGpioSpoofingErrorInjectionPayload` uses
  `ei_gpio_entries[16]` (fixed index) on an array whose size is the
  nondet-bounded `num_of_gpio_entries`. ESBMC CEX: `gpio_ei_entries[16]` OOB.
  Current call site passes a compile-time bound, so not currently exploitable.
- **F-10** (confirmed): `set_busbar_temperature_threshold` silently substitutes
  125 °C when the input is out of range instead of returning an error.
  ESBMC CEX: `threshold=254` → Success returned with silently clamped value.
- **F-13** (latent defect): `DebugTelemetrySmaCh::evaluate` casts `SFXP32_0
  percent` (int32_t) to `UFXP8_0` (uint8_t) without a negative-value guard —
  negative inputs wrap modulo 256. ESBMC CEX: `percent=-1024` → `stored=255`.
  System-level proof (`debug_telemetry_f13_system`, VERIFICATION SUCCESSFUL,
  2081 VCC) shows upstream `std::clamp` in `soc_voltage_to_percent` and
  `OffsetPolicy::run_policy` prevents any negative value from reaching this
  function in production — the defect is unreachable from current data flow.
- **F-14** *retracted*: `Pca9555::i2c_write` bare `return` on the Input command
  was suspected to drop bytes. Production `pca9555.cpp` compiled by ESBMC
  (VERIFICATION SUCCESSFUL, 405 VCC) confirms output state is unchanged — bare
  `return` and `break` are observably equivalent. Code-quality note only.
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
make nsm_type_2_func nsm_type3_func            # ditto
make telemetry_func                            # ditto
make fixed_point_func utils_func               # ditto
make ntc_table_func                            # ditto
make pwr_smooth_params_func fru_utils_func     # ditto
make soc_sma_filter_func debug_telemetry_sma_func  # ditto
make pca9555_func emc1812_func                     # ditto
make tmp1075_func tmp461_func                      # ditto
make volatile                                  # volatile-check phase (all 22 targets)
make mctp_packet_neg mctp_router_neg           # negative tests (expect FAILED)
make mctp_dispatch                             # F-1 reachability proof (expect FAILED)
make literals_neg                              # F-4: _bit(i≥64) UB (expect FAILED)
make nsm_bitmask_neg                           # F-5: set_bit OOB (expect FAILED)
make nsm_type5_f6_neg nsm_type5_f7_neg nsm_type5_f8_neg  # F-6/7/8 (expect FAILED)
make nsm_type3_f10_neg                         # F-10: silent 125°C substitution (expect FAILED)
make debug_telemetry_f13_neg                   # F-13: negative percent wrap (expect FAILED)
make debug_telemetry_f13_system                # F-13 reachability proof (expect SUCCESSFUL)
make pca9555_f14_neg                           # F-14: retracted (expect SUCCESSFUL)
make nsm_event_source_f15_neg                  # F-15: is_event_source_enable OOB (expect FAILED)
make nsm_gpio_safety                           # DCD GPIO structural safety proof (expect SUCCESSFUL)
```

Requires ESBMC on `$PATH` (current `master` recommended), or pass
`ESBMC=/path/to/esbmc make ...`.
