# OpenSMA ESBMC Verification — Initial Report

**Date**: 2026-04-25 (updated 2026-05-04; F-16 added 2026-05-04)
**Tool**: ESBMC 8.2.0 (aarch64-macos)
**Scope**: bounded model checking of selected modules in
[NVIDIA/OpenSMA](https://github.com/NVIDIA/OpenSMA)

## TL;DR

Twenty-two modules verified end-to-end against language-level safety properties
(pointer/bounds/overflow/div-by-zero/memory-leak) and against module-specific
functional contracts via k-induction. **One vulnerability formally proven
reachable** (F-1) via `mctp_dispatch` — ESBMC finds a counterexample where a
Control SetEpId Request with a gap interface triggers `set_cur_eid()` to
OOB-index the 2-entry `cur_eid` array. **Four additional security findings
formally confirmed** by ESBMC (VERIFICATION FAILED on dedicated negative
harnesses) and independently reproduced by native execution under address /
undefined-behaviour sanitizers: **F-6** (unchecked mode byte in
`on_dev_cfg_set_errorInjectionMode`), **F-7** (no rollback after
`PortRecoveryPayload` validation failure), **F-8** (OOB in
`validateGpioSpoofingErrorInjectionPayload`), **F-13** (negative percent
wrap in `DebugTelemetrySmaCh`). Six findings retracted: three after ESBMC
returned VERIFICATION SUCCESSFUL (**F-9**, **F-12**, **F-14**) and three on
initial review (**F-2**, **F-3**, **former F-6**) — see [Retracted findings](#retracted-findings)
and [Appendix A](#appendix-a--retraction-details). Two confirmed latent-UB
findings: **F-4** in `literals.h::operator""_bit` (shift-count ≥ 64) and
**F-5** in `nsm_msg_bitmask.h::set_bit` / `unset_bit` on the 8-element event
bitmask (index ≥ 64 reaches `std::array::at` OOB). Neither F-4 nor F-5 has a
dangerous current call site, but F-5 lacks the runtime guard that sibling
operations carry. An exhaustive call-graph trace confirmed that every
`set_bit(8-element, pos)` call site uses a compile-time enum constant — F-5 is
confirmed latent with no current packet-driven path. A third latent finding, **F-10** (`set_busbar_temperature_threshold` silent 125 °C
substitution), is dead code in all current builds: `BusBarTempSensorNum = 0` on every
known platform config (`p3957_cxx`, `testrunner`, `mcxn547helloworld`) causes the
`if constexpr` block at `nsm_type_3.cpp:447` to be compiled away; the function returns
`Ccode::Success` unconditionally in production. A fourth latent-OOB finding, **F-15**,
confirms the same asymmetric-guard pattern on the read path:
`is_event_source_enable()` reads `type0/6_event_enable_bitmask.at(event_id/8)`
without a bounds check (ESBMC CEX: `event_id=248`, `ByteIndex=31`, OOB on
size-8 array). A fifth latent-OOB finding, **F-16**, is the same pattern in
the sibling function `is_event_ack_enable()` (`nsm.cpp:1089`), which reads
`type0/6_event_ack_bitmask.at(event_id/8)` on the same size-8 arrays with no
bounds check (ESBMC CEX: `event_id=248`, `ByteIndex=31`, OOB). A concurrent sweep of the DCD GPIO handlers (`on_dcd_get_gpio` /
`on_dcd_set_gpio`) produced VERIFICATION SUCCESSFUL (536 VCC): the
`(offset+length) > GpioNum` guard is sufficient to keep all `GpioSetup.at()`
and `gpio_resp.gpio.at()` accesses in bounds. Several ESBMC C++-frontend bugs
filed against
[esbmc/esbmc](https://github.com/esbmc/esbmc); most are now fixed and merged;
workarounds removed where applicable.

## What was verified

| Target | Source | Phase 1 | Phase 2 | Negative test |
|---|---|:-:|:-:|:-:|
| MCTP packet parser | `corepdk/.../app/pdk-mctp-app-packet.cpp` | ✅ 62 VCC | ✅ 76 VCC, k=1 | ✅ CEX on undersized input |
| MCTP routing helpers | `corepdk/.../platforms/x86/pdk-mctp-platforms-router-plat.cpp` | ✅ 70 VCC | ✅ 66 VCC, k=1 | ✅ CEX on `iface == UsEnd` |
| MCTP dispatch (F-1 reachability) | `Validator::validate()` + `VerifControl::on_set_endpoint_id()` (production) | — | — | ✅ CEX: `iface_val=2`, `valid=true`, OOB at `cur_eid.at(2)` (275 VCC) |
| Fixed-point arithmetic | `src/nv/common/fixed_point.h` | ✅ 78 VCC | ✅ 24 VCC, k=1 | — |
| Saturating arithmetic | `src/nv/common/utils.h` | ✅ 20 VCC | ✅ k=1 | ⚠ ESBMC strict unsigned-overflow demo (not a bug) |
| MCTP validator state machine | `corepdk/.../app/pdk-mctp-app-validator.cpp` | ✅ 119 VCC | ✅ k=1 (full functional contract) | — |
| NSM type 2 (PCIe-link reset validator) | `src/nv/mctp/nsm_type_2.cpp` (`validatePcieLinkResetValue`) | ✅ | ✅ k=12 (membership iff + below-range rejection) | — |
| NSM type 3 sensor availability | `src/nv/mctp/nsm_type_3.cpp` (`is_temp_sensor_available`, `is_power_sensor_available`, `is_voltage_sensor_available`) | ✅ 37 VCC | ✅ k=9 (membership iff, busbar-unavailable exclusion, voltage always-false) | ⚠ **F-10** CEX: `threshold=254` → Success for out-of-range temp — dead code in all current builds (`BusBarTempSensorNum = 0`; `if constexpr` body compiled away) |
| Telemetry sensor-ID lookup + LE deserialiser | `src/nv/telemetry/utils.h` (`getTelemIdFromTempSensorId`, `getTelemIdFromPowerSensorId`, `buffer_to_uint32`) | ✅ 80 VCC | ✅ k=11 (mapping iff, MaxItem for non-members, LE byte-order contract) | — |
| SPI byte-buffer (de)serialisation | `src/nv/spi/utils.{h,cpp}` (`buf_to_u{16,32}`, `u{16,32}_to_buf`) | ✅ | ✅ k=9 (round-trip + big-endian + OOB-no-write) | — |
| I2C CRC-8 helpers | `src/nv/i2c/helper.cpp` (`crc8`) | ✅ | ✅ k=5 (incrementality + init-zero invariant) | — |
| User-defined integer literals | `src/nv/common/literals.h` (`_u8`/`_u16`/`_u32`/`_i8`/`_i16`/`_i32`/`_bits_sizeof`/`_bit`) | ✅ | ✅ k=1 (mask agreement, signed/unsigned truncation parity, `bits/8`, `1ULL << i`) | ✅ CEX on `_bit(i≥64)` via `--ub-shift-check` — **F-4** |
| NSM bitmask operations | `src/nv/mctp/nsm_msg_bitmask.h` (`set_bit`/`unset_bit`/`get_bit`/`is_bit_set`) | ✅ 75 VCC | ✅ k=1 (set→get non-zero; unset→get zero; is_bit_set iff get_bit≠0) | ✅ CEX on `set_bit`/`unset_bit(arr8, pos≥64)` — **F-5** |
| NSM type 5 field validators | `src/nv/mctp/nsm_type_5.cpp` (`validateFatalErrorInjectionPayload`, `validateDeviceIndex{GpuDegradeMode,PowerSupply}`, `validateAction{GpuDegradeMode}`, `validateModePowerSupply`) | ✅ 14 VCC | ✅ k=1 (exact characterisation: accepted iff bitmask∈{0,1,2}, index/mode in documented ranges) | ✅ **F-6** CEX: `mode=0xFF` stored; system-level: real `process_device_configuration` compiled, `mode=3` stored — **nsm_f6_system** FAILED; **F-7** CEX: dirty `portRecoveryResp` on validation failure; system-level: real `process_device_configuration` compiled, validator always returns true — **nsm_f7_system** SUCCESSFUL (confirmed latent); **F-8** CEX: `gpio_ei_entries[16]` OOB |
| NTC thermistor table | `src/nv/volt_mon/ntc_table.{h,cpp}` (`ntc_resistance_to_temperature`, `ntc_voltage_to_temperature`, `ntc_adc_to_temperature`, `ntc_temperature_to_resistance`, `ntc_temp_to_adc_value`) | ✅ 227 VCC | ✅ k=9 (exact table lookup, range clamping, round-trip identity) | — |
| Power-smoothing params | `src/nv/soc_pwr_smoothing/presets.{h,cpp}` (`OverrideParam::to_uint32`, `::from_uint32`, `is_valid_param_id`) | ✅ 72 VCC | ✅ k=1 (round-trip pack↔unpack identity, param-id exact characterisation) | — |
| FRU utilities | `src/nv/fru/fru.cpp` (`verify_checksum`, `decode_6bit_ascii`) | ✅ 76 VCC | ✅ k=9 (checksum true iff sum≡0 mod 256, decode output ∈ [0x20, 0x5F]) | — |
| SoC SMA filter | `src/nv/soc_pwr_smoothing/soc_sma_filter_ch.h` (`SocSmaFilterCh::evaluate` — 4-sample sliding-window SMA over SFXP22_10) | ✅ 504 VCC | ✅ k=1 (steady-state: 4 equal inputs → output == input; output ∈ [0, input]) | — |
| Debug telemetry SMA | `src/nv/soc_pwr_smoothing/debug_telemetry_sma_ch.h` (`DebugTelemetrySmaCh::evaluate` — 256-sample SMA; UFXP8_0 buffer; percent ∈ [0%, 150%]) | ✅ 261 VCC | ✅ k=1 (index bounded ∈ [0, 255] by bitwise-AND; output non-negative from zero state) | ✅ **F-13** CEX: `percent=-1024` → `stored=255` (negative wrap); ✅ **F-13 system** VERIFICATION SUCCESSFUL (2081 VCC, 284 post-simplification): upstream `std::clamp` in `soc_voltage_to_percent` and `OffsetPolicy::run_policy` prevents negative inputs from ever reaching `DebugTelemetrySmaCh` — latent defect, unreachable in production |
| PCA9555 GPIO expander emulator | `src/nv/emulation/pca9555.{h,cpp}` (`Pca9555` — 16-bit I2C GPIO expander; direction/input/output/inversion registers + interrupt-on-change logic) | ✅ 1292 VCC | ✅ k=2 (direction constraint with precondition req_in∩req_out=∅; input_update_masked; interrupt_default; output_propagation) | ✅ VERIFICATION SUCCESSFUL (405 VCC, production `pca9555.cpp`): output state unchanged after Input-register write — **F-14 retracted** (bare `return` and `break` observably equivalent; code-quality note) |
| EMC1812 temperature sensor driver | `src/nv/i2c/emc1812.{h,cpp}` (`Emc1812` — EMC1812 temp sensor driver; all public methods with nondet I2C stubs; `int8_t↔uint8_t` threshold cast round-trip verified for all six set/get pairs) | ✅ 52 VCC | ✅ k=1 (cast_roundtrip: `static_cast<int8_t>(static_cast<uint8_t>(t)) == t` for all `int8_t t`; threshold_symmetry: all four pairs) | — |
| TMP1075 temperature sensor driver | `src/nv/i2c/tmp1075.{h,cpp}` (`Tmp1075` — 12-bit two's-complement temperature encoding: `int8_t → <<4 → int16_t → uint16_t → >>4 → int8_t` round-trip; `get_device_id`; `set/get_{low,high}_limit`) | ✅ 33 VCC | ✅ k=1 (12bit_roundtrip: `static_cast<int8_t>(static_cast<int16_t>(static_cast<uint16_t>(static_cast<int16_t>(t<<4)))>>4) == t` for all `int8_t t`; temp_read_cast well-defined) | — |
| TMP461 temperature sensor driver | `src/nv/i2c/tmp461.{h,cpp}` (`Tmp461` / NCT72 — `int8_t↔uint8_t` threshold cast round-trip for four alert/therm set/get pairs; `get_configuration`) | ✅ 57 VCC | ✅ k=1 (cast_roundtrip + threshold_symmetry for all four pairs) | — |
| `is_event_source_enable` OOB check (F-15) | `src/nv/mctp/nsm.cpp:761` — `type0/6_event_enable_bitmask.at(event_id/8)` without bounds guard; same asymmetric-guard pattern as F-5 but on the read path | — | — | ✅ VERIFICATION FAILED — CEX: `event_id=248`, `ByteIndex=31`, OOB at `at()` on size-8 array — **F-15** |
| `is_event_ack_enable` OOB check (F-16) | `src/nv/mctp/nsm.cpp:1089` — `type0/6_event_ack_bitmask.at(event_id/8)` without bounds guard; sibling function to F-15, same pattern | — | — | ✅ VERIFICATION FAILED — CEX: `event_id=248`, `ByteIndex=31`, OOB at `at()` on size-8 array — **F-16** |
| DCD GPIO safety proof | `src/nv/mctp/nsm.cpp:3389,3482` — `on_dcd_get_gpio` / `on_dcd_set_gpio`; guard `(offset+length) > GpioNum` keeps all `GpioSetup.at()` and `gpio_resp.gpio.at()` in bounds | — | — | ✅ VERIFICATION SUCCESSFUL (536 VCC) — **no defect** |

All BMC runs solved sub-second on Bitwuzla 0.8.2.

### Properties checked

**Phase 1 — language-level safety** (default + opt-in checks):

- pointer-check, bounds-check, div-by-zero (default-on)
- arithmetic over/underflow (signed and unsigned)
- memory-leak
- NaN propagation
- loop exhaustion (unwinding assertions — default ESBMC behaviour; each
  target's `--unwind N` is set to the exact loop bound so the assertion
  closes, not cut off)

**Phase 2 — functional contracts** via `--k-induction --k-step 1
--max-k-step 6`:

- `Packet`: `to_span` / `from` round-trip preserves all fields; the resulting
  span's size equals `priv + hdr + msg`.
- `RoutingTable`: `get_cur_eid(set_cur_eid(t, i, eid), i) == eid` for all
  `i < UsEnd`.
- `fixed_point`: `sfxp22_10_to_sfxp32_0(sfxp32_0_to_sfxp22_10(x)) == x` for
  all `x ∈ [INT32_MIN >> 10, INT32_MAX >> 10]`.

## Findings

### Proof tiers

Findings are ordered from highest to lowest proof rigor.

| Tier | Criteria | Findings |
|---|---|---|
| **A** | Production source compiled end-to-end; full packet dispatch chain proven from network input; runtime abort confirmed | F-1 |
| **B** | Production source compiled; handler-level CEX proven; directly reachable | F-6 |
| **C** | Structural CEX + sanitizer-confirmed + system-level latency proof (production code compiled) | F-13, F-7 |
| **D** | Structural CEX + sanitizer-confirmed; latent (existing call site carries a guard) | F-8 |
| **E** | Structural CEX; latent; exhaustive call-graph trace confirms no current packet-driven path | F-5 |
| **F** | Structural CEX; latent; no current dangerous call site | F-15, F-16, F-4 |
| **G** | Structural (partial production source compiled); dead code in all current builds | F-10 |

---


### F-1 — `set_cur_eid()` lacks bounds check (reachability formally proven) *(Tier A)*

**File**: `corepdk/modules/mctp-cpp/src/platforms/x86/pdk-mctp-platforms-router-plat.cpp:34`

```cpp
void set_cur_eid(RoutingTable& routing_table,
                 Packet::InterfaceType interface, uint8_t eid)
{
    routing_table.ec.cur_eid.at(interface) = eid;
}
```

`cur_eid` is a `std::array<uint8_t, UsEnd>` (`UsEnd == 2`). The sibling
`get_cur_eid` guards with `if (interface >= UsEnd) ...`; `set_cur_eid` has
no such guard.

**Root cause — the validator gap**: `Validator::validate()` rejects
`interface >= Interface::End` (18), but `cur_eid` is sized to
`Interface::UsEnd` (2). Any interface value in `[2, 17]` — the gap — passes
validation yet OOBs in `set_cur_eid()`.

**What ESBMC proved** (`mctp_dispatch` harness, `LANG_FLAGS`, 275 VCC / 16
remaining after simplification; Bitwuzla solves in <0.01 s):

The harness constrains `iface_val` to `[UsEnd=2, End=18)` and constructs a
**Control SetEpId Request** — the exact packet type dispatched to
`on_set_endpoint_id()` — satisfying every guard inside `validate()`
(`hdr_ver=1`, `dst_eid=NULL_EID`, `som=1`, `eom=1`, `tag_owner=1`,
`msg_type=Control`, `rq=1`, `Cmd::SetEpId`, `SetEidNormal`). ESBMC finds the
counterexample immediately:

```
State 5   iface_val = 2   (first gap value: passes assume >= UsEnd && < End)
State 9   valid = 1       (Validator::validate() returns true)
State 10  Violated: std::array::at out of range  [i::0 < 2  fails]
          at set_cur_eid() → cur_eid.at(2) on a size-2 array
```

Any `iface_val ∈ [2, 17]` triggers the same path.

**Production connection (formally proven)**: the harness compiles
`pdk-mctp-platforms-control.cpp` as-is and calls the production
`on_set_endpoint_id()` directly via a `VerifControl` thin subclass
that promotes the protected method to public. The `VerifControl`
subclass gives the `Validator` the same `RoutingTable` (`ctrl.router()`),
so both steps operate on the same object. This eliminates all code-inspection
caveats — the full dispatch path from `validate()` through
`on_set_endpoint_id()` to `set_cur_eid()` is now formally proven end-to-end
with the production source unchanged.

**Runtime effect (empirically confirmed)**: under production flags
`-fno-exceptions -fno-rtti`, `std::array::at(OOB)` calls `abort()` — not
silent corruption, not a throw. Reproduced via ESBMC's `--branch-coverage
--generate-ctest-testcase` on a 26-line standalone harness (`ctest/f1/`);
the generated executable exits with signal 6 (SIGABRT).

**Recommendation**: this is a confirmed abort path for any Control SetEpId
Request whose `priv.packet_interface` falls in `[UsEnd, End)`. Add the guard
that `get_cur_eid` already carries:

```cpp
if (interface >= static_cast<uint16_t>(Interface::UsEnd)) {
    return;
}
routing_table.ec.cur_eid.at(interface) = eid;
```

Alternatively, tighten `Validator::validate()` to reject `interface >=
Interface::UsEnd` instead of `>= Interface::End`.

### F-6 — `on_dev_cfg_set_errorInjectionMode` stores unchecked mode byte *(Tier B)*

**File**: `src/nv/mctp/nsm_type_5.cpp:777–803`

```cpp
Ccode on_dev_cfg_set_errorInjectionMode(const NsmRequest& nrx)
{
    ...
    type5_data.errorInjectionModeResponse.mode = nrx.data[0];  // no range check
    ...
}
```

`nrx.data[0]` is an arbitrary byte from the network. `NsmDevCfgEnablingMode` has only two valid values (`Disable = 0x00`, `Enable = 0x01`), but the field is stored without validation. Any out-of-range value (e.g. `0xFF`) is accepted and persisted.

**What ESBMC proved** (`nsm_type5_f6_neg`, VERIFICATION FAILED):

Nondet `request_mode` constrained to `request_mode != Disable && request_mode != Enable` (i.e. any value other than 0 or 1). The harness asserts `mode ∈ {Disable, Enable}` after the write. CEX: `mode = 0xFF` stored.

**System-level proof** (`nsm_f6_system`, VERIFICATION FAILED):

The real `Nsm::process_device_configuration()` compiled from production `nsm_type_5.cpp` (not an inline copy). Nondet `nrx.data[0]`; no constraint. CEX traces `nondet_symbol` from `nrx.data[0]` (harness:49) through `process_device_configuration` (nsm_type_5.cpp:647) → `on_dev_cfg_set_errorInjectionMode` (nsm_type_5.cpp:801) to `type5_data.errorInjectionModeResponse.mode`. Assertion `mode ∈ {Disable, Enable}` violated with `mode = 3` (`--unwind 9`, `--no-align-check` — the latter suppresses a residual false positive on the `[[gnu::packed]]` bitfield constructor `NsmDevCfgErrorInjectionModeResponse()`, tracked as esbmc#4267).

**Runtime confirmation**: sanitizer run (`-fsanitize=address,undefined`) with `request_mode = 0xFF` triggers `assert(mode == Disable || mode == Enable)` → SIGABRT. `ctest/f6/`.

**Recommendation**: add a range check before the assignment:
```cpp
if (nrx.data[0] != Disable && nrx.data[0] != Enable)
    return Ccode::ErrorInvalidData;
type5_data.errorInjectionModeResponse.mode = nrx.data[0];
```

---

### F-13 — `DebugTelemetrySmaCh::evaluate` wraps negative percent to unsigned *(Tier C)*

**File**: `src/nv/soc_pwr_smoothing/debug_telemetry_sma_ch.h`

```cpp
// sfxp22_10_to_sfxp32_0 converts SFXP22.10 → SFXP32.0 (i.e. >> 10, signed)
const auto percent_int = sfxp22_10_to_sfxp32_0(ports.percent);   // SFXP32.0
static_cast<UFXP8_0>(percent_int)                                  // → uint8_t
// stored in SMA buffer
```

`ports.percent` is a signed fixed-point value. `sfxp22_10_to_sfxp32_0` applies a signed right-shift (`>> 10`). If the resulting `SFXP32_0` integer is negative (e.g. `-1`), `static_cast<UFXP8_0>` (which is `uint8_t`) wraps modulo 256: `-1 → 255`. The SMA buffer then stores `255` when the true value was `−1` (≈ `-0.001%`), corrupting any downstream smoothed-percentage computation.

**What ESBMC proved** (`debug_telemetry_f13_neg`, VERIFICATION FAILED):

Harness includes the **production header** `nv/soc_pwr_smoothing/debug_telemetry_sma_ch.h` and calls `filter.evaluate(ports)` directly. Nondet `percent` constrained to `x < 0`. Asserts `stored <= 150`. CEX: `percent = -1024` → `sfxp32_0 = -1` → `stored = 255 > 150`.

**Runtime confirmation**: sanitizer run with `x = -1024` → assertion fires. `ctest/f13/`.

**Recommendation**: guard against negative percent before cast:
```cpp
if (percent_int < 0)
    return;                     // or clamp to 0
static_cast<UFXP8_0>(percent_int);
```

**Reachability proof** (`debug_telemetry_f13_system`, VERIFICATION SUCCESSFUL — 2081 VCC, 284 post-simplification):

System-level harness calls the real `PowerManager::run_iteration()` (production code, not an inline model) with nondet ADC reading and nondet GPIO (thermal warning asserted/deasserted). All three `DebugTelemetrySmaCh` outputs are asserted non-negative:

```
__ESBMC_assert(
    pm.public_connectors.soc_percent_avg  >= static_cast<SFXP22_10>(0)
 && pm.public_connectors.edpp_offset_avg  >= static_cast<SFXP22_10>(0)
 && pm.public_connectors.isink_offset_avg >= static_cast<SFXP22_10>(0), ...);
```

ESBMC proves no counterexample exists for any hardware input. The three upstream paths each prevent negative values from reaching `DebugTelemetrySmaCh::evaluate()`:

- `soc_percent_filtered`: `StateOfChargeDev::soc_voltage_to_percent()` applies `std::clamp(%, 0, 100)` before the value enters the SMA.
- `edpp_offset`: `OffsetPolicy::run_policy<Edpp>` returns `std::clamp(critical+residency, 0, 100)`; reset paths return `0`.
- `isink_offset`: `OffsetPolicy::run_policy<Isink>` returns `100 - std::clamp(…)` ∈ [0, 100]; reset paths return `100` or `0`.

**Conclusion**: F-13 is a **latent defect**. The function `DebugTelemetrySmaCh::evaluate()` is unsafe when called with negative input, but the current data flow prevents that from ever happening. The recommendation above remains valid as a defensive hardening.

---

### F-7 — `on_dev_cfg_set_portRecoveryErrorInjection` writes before validating (no rollback) *(Tier C)*

**File**: `src/nv/mctp/nsm_type_5.cpp:1085–1108`

```cpp
static Ccode on_dev_cfg_set_portRecoveryErrorInjection(const NsmRequest& nrx)
{
    memcpy(&portRecoveryEIPayload, nrx.data, sizeof(portRecoveryEIPayload));  // write first
    if (!validatePortRecoveryErrorInjectionPayload(...)) {
        // portRecoveryEIPayload already corrupted — no rollback
        return Ccode::ErrorInvalidData;
    }
    ...
}
```

The handler copies the incoming payload into `portRecoveryEIPayload` (persistent state) **before** validating it. If validation fails, the function returns an error but `portRecoveryEIPayload` already holds the invalid data. A subsequent read of `portRecoveryEIPayload` will observe the corrupted value.

**What ESBMC proved** (`nsm_type5_f7_neg`, VERIFICATION FAILED):

Nondet `incoming` payload, nondet validator constrained to fail (`!valid`). After the failed write, harness asserts `is_zero_initialised(stored)` — that `portRecoveryEIPayload` is unchanged from its zero-initialised state. CEX: `incoming.offset = 42`, validator returns false, `stored.offset = 42`.

**System-level proof** (`nsm_f7_system`, VERIFICATION SUCCESSFUL):

The real `Nsm::process_device_configuration()` compiled from production `nsm_type_5.cpp` (not an inline copy). Packet crafted with `SetErrorInjectionPayload` / `PortRecoveryErrors` (OCP v2, DeviceError id, nondet bitmaps). ESBMC reports **VERIFICATION SUCCESSFUL** (`--unwind 13`, same `--no-align-check` workaround as F-6 system): all reachable paths are memory-safe and overflow-free; the validation-failure branch is dead code because `validatePortRecoveryErrorInjectionPayload` always returns `true` (production TODO stub, nsm_type_5.cpp:206–210). This closes gap-1 (real `PortRecoveryPayload`/`NsmDevCfgPersistentData` types) and gap-2 (real dispatch logic) from the structural harness. Gap-3 (validator as nondet bool) cannot be closed without production code changes — the validator must be completed before F-7 becomes reachable. **Confirmed latent in current production code.**

**Runtime confirmation**: sanitizer run with `incoming.offset = 42` and validator forced to return false → assertion fires. `ctest/f7/`.

**Recommendation**: validate before writing, or save and restore on failure:
```cpp
// Option A: validate-then-write
if (!validatePortRecoveryErrorInjectionPayload(...))
    return Ccode::ErrorInvalidData;
memcpy(&portRecoveryEIPayload, nrx.data, sizeof(portRecoveryEIPayload));
```

---

### F-8 — `validateGpioSpoofingErrorInjectionPayload` lacks bounds check on `ei_gpio_entries` (latent) *(Tier D)*

**File**: `src/nv/mctp/nsm_type_5.cpp:298–333`

```cpp
for (uint8_t i = 0; i < gpioSpoofingPayload.header.ei_gpio_number; i++) {
    auto gpio_entry = gpioSpoofingPayload.data.ei_gpio_entries[i];  // no bounds check
    ...
}
```

`ei_gpio_entries` is sized `MaxGPIOSpoofingEntries = 16`. The loop iterates `ei_gpio_number` times without first checking `ei_gpio_number <= MaxGPIOSpoofingEntries`. A crafted payload with `ei_gpio_number = 17` would access `ei_gpio_entries[16]`, one element past the array end.

**What ESBMC proved** (`nsm_type5_f8_neg`, `--unwind 17`, VERIFICATION FAILED):

Nondet `n` constrained to `n > MaxGPIOSpoofingEntries`; loop runs to `i = 16`. CEX: `ei_gpio_entries[16]` OOB access at `i = 16`.

**Classification: latent issue.** The call site at `nsm_type_5.cpp:1147–1150` includes a guard:
```cpp
if (gpioSpoofingHeader.ei_gpio_number > MaxGPIOSpoofingEntries)
    return Ccode::ErrorInvalidData;
```
This guard prevents the OOB from being directly reachable in the current codebase. The finding is latent: the validator itself is unsafe and could be called from a future call site without the guard.

**Recommendation**: add the bounds check inside `validateGpioSpoofingErrorInjectionPayload` so the invariant is self-contained and does not depend on caller discipline:
```cpp
if (gpioSpoofingPayload.header.ei_gpio_number > MaxGPIOSpoofingEntries)
    return false;
```

---

### F-5 — `set_bit` / `unset_bit` missing bounds guard on the 8-element event bitmask *(Tier E)*

**File**: `src/nv/mctp/nsm_msg_bitmask.h`

```cpp
// For NSM Events (NvMctpEventSupportedNum = 8 bytes = 64 bits)
static constexpr void set_bit(std::array<uint8_t, NvMctpEventSupportedNum>& bitmask,
                              uint8_t pos)
{
    const size_t byte_index  = pos / 8;          // 0–31 for uint8_t pos
    const size_t bit_offset  = pos % 8;
    bitmask.at(byte_index)  |= ...;              // OOB if byte_index >= 8
}
```

`bitmask.at(byte_index)` throws (or calls `abort()` under `-fno-exceptions`)
when `byte_index >= NvMctpEventSupportedNum (8)`, i.e. when `pos >= 64`. The
sibling `get_bit` carries `if (byte_index < bitmask.size()) ...` which prevents
the OOB; `set_bit` and `unset_bit` have no equivalent guard.

The same asymmetry exists for the 32-element (`NvMctpSupportedNum`) overloads,
but for that size `byte_index = pos/8 ≤ 31 < 32` holds for all `uint8_t pos`,
so the 32-element `set_bit` is safe for every possible argument.

**What ESBMC proved** (`nsm_bitmask_neg`, nondet `pos ∈ [64, 255]`):

```
State 2  pos = 248
State 6  Violated: Index out of bounds
         index::0 < 8    (byte_index = 248/8 = 31 on a size-8 array)
VERIFICATION FAILED
```

**Severity: low in practice.** All current call sites pass small compile-time
constants: `static_cast<uint8_t>(DeviceError)` = 4 and
`static_cast<uint8_t>(GpioSpoofing)` = 5 (`nsm_type_5.h:125–126`). Both map
to `byte_index = 0`, well within the 8-element array. No current call site
passes a runtime or loop-controlled position.

The risk is latent: a future caller that iterates over error-type positions or
passes an unvalidated `uint8_t` field (e.g. received from a packet) could reach
`pos ≥ 64` and trigger the abort path under production `-fno-exceptions` flags.

**Recommendation**: add the same bounds guard that `get_bit` already carries:

```cpp
static constexpr void set_bit(std::array<uint8_t, NvMctpEventSupportedNum>& bitmask,
                              uint8_t pos)
{
    const size_t byte_index = pos / 8;
    if (byte_index >= bitmask.size())
        return;                                  // matches get_bit's guard
    const size_t bit_offset  = pos % 8;
    bitmask.at(byte_index)  |= static_cast<uint8_t>((1U << bit_offset) & UINT8_MAX);
}
```

Apply the same fix to `unset_bit`. Alternatively, make the precondition
explicit in a `static_assert` or `constexpr` wrapper that limits `pos` to the
representable range of the array.

**Call graph investigation — confirmed latent, no packet-driven path**: An
exhaustive trace of every `set_bit(NvMctpEventSupportedNum=8, pos)` call site
in the production source tree found three locations, all using compile-time
enum constants:

| Call site | File | `pos` value |
|---|---|---|
| `gen_type5_supported_errors_injection_bitmask()` | `nsm_type_5.h:125` | `DeviceError = 4` |
| `gen_type5_supported_errors_injection_bitmask()` | `nsm_type_5.h:126` | `GpioSpoofing = 5` |
| `gen_type6_event_bitmask()` | `nsm.h:1379` | `NsmFwEvent::RotStateInformationChangeEvent = 1` |

Packet handlers that write to 8-element bitmasks bypass `set_bit` entirely:
`on_dev_cfg_set_currentErrorInjectionTypes` (`nsm_type_5.cpp:755`) copies the
8-byte payload with `memcpy` and then iterates with `at(index)` for `index ∈
[0, 7]`; `on_dcd_set_current_event_srcs` (`nsm.cpp:983`) does the same.
Neither calls `set_bit` at runtime. A Tier 2 system-level harness for F-5
would therefore produce VERIFICATION SUCCESSFUL (confirmed latent), not FAILED.
F-5 cannot be reported at F-1's level of certainty without a future code change
that routes an unvalidated `pos` value through the write path.

### F-15 — `is_event_source_enable` missing bounds check on `event_id` *(Tier F)*

**File**: `src/nv/mctp/nsm.cpp:761–799`

```cpp
bool Nsm::is_event_source_enable(NsmMsgType msg_type, uint8_t event_id) const
{
    const size_t ByteIndex = event_id / 8;
    const size_t BitOffset = event_id % 8;

    if (msg_type == NsmMsgType::DeviceCapabilityDiscovery)
        return (type0_event_enable_bitmask.at(ByteIndex) & (1U << BitOffset)) != 0;
    else
        return (type6_event_enable_bitmask.at(ByteIndex) & (1U << BitOffset)) != 0;
}
```

`type0_event_enable_bitmask` and `type6_event_enable_bitmask` are both
`std::array<uint8_t, NvMctpEventSupportedNum=8>`. For `event_id ∈ [64, 255]`,
`ByteIndex = event_id/8 ∈ [8, 31]` — out of range for a size-8 array.
`std::array::at()` calls `abort()` under production `-fno-exceptions`. No bounds
guard is present, unlike `get_bit()` in `nsm_msg_bitmask.h` which carries
`if (byte_index < bitmask.size()) ...`.

This is the same asymmetric-guard pattern as F-5 (`set_bit`/`unset_bit` missing
guard) but on the read path.

**What ESBMC proved** (`nsm_event_source_f15_neg`, `LANG_FLAGS`, nondet `event_id ≥ 64`):

```
State 2   event_id = 248
State 5   ByteIndex = 31
State 9   Violated: Index out of bounds
          index::0 < 8    (ByteIndex=31 on a size-8 array)
VERIFICATION FAILED
```

**Severity: low in practice.** The function is called from two sites:

- `nsm_event.cpp:53` (`PrepareEventMessage`): `eventId` originates from IPC queue
  field `Cmd.data2` — an internal message between firmware tasks, not a raw packet
  field. Range of `eventId` in production is bounded by the event-type enum
  (`NsmFwEvent`, values 0–15), well below 64.
- `driver.cpp:807` (`is_event_source_enable`): called in a `switch` with only
  known enum values.

No current call site passes a runtime value that can reach ≥ 64. The risk is
latent: a future handler that passes an unvalidated `uint8_t` from a packet or
IPC field to `is_event_source_enable` would trigger the abort path.

**Recommendation**: add the same guard that `get_bit` already carries:

```cpp
bool Nsm::is_event_source_enable(NsmMsgType msg_type, uint8_t event_id) const
{
    const size_t ByteIndex = event_id / 8;
    if (ByteIndex >= type0_event_enable_bitmask.size())
        return false;
    const size_t BitOffset = event_id % 8;
    if (msg_type == NsmMsgType::DeviceCapabilityDiscovery)
        return (type0_event_enable_bitmask.at(ByteIndex) & (1U << BitOffset)) != 0;
    else
        return (type6_event_enable_bitmask.at(ByteIndex) & (1U << BitOffset)) != 0;
}
```

---

### F-16 — `is_event_ack_enable` missing bounds check on `event_id` *(Tier F)*

**File**: `src/nv/mctp/nsm.cpp:1089–1106`

```cpp
bool Nsm::is_event_ack_enable(NsmMsgType nv_msg_type, uint8_t event_id)
{
    if (nv_msg_type == NsmMsgType::Firmware) {
        const size_t ByteIndex = event_id / 8;
        const size_t BitOffset = event_id % 8;
        return (type6_event_ack_bitmask.at(ByteIndex) & (1u << BitOffset)) != 0;
    }
    else if (nv_msg_type == NsmMsgType::DeviceCapabilityDiscovery) {
        const size_t ByteIndex = event_id / 8;
        const size_t BitOffset = event_id % 8;
        return (type0_event_ack_bitmask.at(ByteIndex) & (1u << BitOffset)) != 0;
    }
    else { return false; }
}
```

`type0_event_ack_bitmask` and `type6_event_ack_bitmask` are both
`std::array<uint8_t, NvMctpEventSupportedNum=8>`. For `event_id ∈ [64, 255]`,
`ByteIndex = event_id/8 ∈ [8, 31]` — out of range. `std::array::at()` calls
`abort()` under `-fno-exceptions`. Same asymmetric-guard pattern as F-5 and F-15.

**What ESBMC proved** (`nsm_event_ack_f16_neg`, `LANG_FLAGS`, nondet `event_id ≥ 64`):

```
State 2   event_id = 248
State 5   ByteIndex = 31
State 11  Violated: dereference failure: Access to object out of bounds
          (ByteIndex=31 on a size-8 array)
VERIFICATION FAILED
```

**Severity: low in practice.** The sole call site is `nsm_event.cpp:96`
(`PrepareEventMessage`), which is called from `driver.cpp:227`
(`Driver::on_receive_event`). The `eventId` parameter is a `uint8_t` from
an IPC field — an internal message between firmware tasks, not a raw network
packet. In the current codebase `is_event_source_enable` (F-15) is checked
first at `nsm_event.cpp:53`; passing that guard does not constrain `event_id`
to `< 64`, so a future event source with `event_id ≥ 64` would reach F-16
immediately after passing F-15.

**Recommendation**: add the same guard that `get_bit` carries:

```cpp
bool Nsm::is_event_ack_enable(NsmMsgType nv_msg_type, uint8_t event_id)
{
    const size_t ByteIndex = event_id / 8;
    if (ByteIndex >= type0_event_ack_bitmask.size())
        return false;
    const size_t BitOffset = event_id % 8;
    if (nv_msg_type == NsmMsgType::Firmware)
        return (type6_event_ack_bitmask.at(ByteIndex) & (1u << BitOffset)) != 0;
    else if (nv_msg_type == NsmMsgType::DeviceCapabilityDiscovery)
        return (type0_event_ack_bitmask.at(ByteIndex) & (1u << BitOffset)) != 0;
    else
        return false;
}
```

---

### F-4 — `operator""_bit` missing precondition guard on shift count *(Tier F)*

**File**: `src/nv/common/literals.h:61`

```cpp
constexpr auto operator""_bit(unsigned long long i)
{
    return static_cast<decltype(i)>(1) << i;
}
```

`1ULL << i` is undefined behaviour when `i >= 64` — the shift count equals or
exceeds the width of `unsigned long long` (`[expr.shift]/1`). The function is
`constexpr` but not `consteval`, so a runtime invocation with an out-of-range
argument is valid C++ that silently invokes UB.

**What ESBMC proved** (`literals_neg`, `--ub-shift-check`, nondet `i ∈ [64, 128)`):

```
State 1  i = 64
State 3  Violated: undefined behavior on shift operation shl
         i::0 < 64  (shift count must be < type width)
VERIFICATION FAILED
```

**Severity: low in practice.** Every production call site uses a small
compile-time constant as the UDL operand (e.g. `1_bit`, `2_bit`, `3_bit` in
enum class definitions across `spi_edma.h`, `ssif.h`, `i2c_types.h`, etc.).
The compiler evaluates those at compile time and would diagnose any
out-of-range literal. No current call site passes a runtime value.

The risk is latent: a future caller that loops over bit positions (e.g.
`for (int b = 0; b < N; ++b) mask |= nv::operator""_bit(b)`) would silently
invoke UB once `b >= 64`.

**Recommendation**: change `constexpr` to `consteval` — this locks the
operator to compile-time-only use at zero runtime cost and eliminates the
concern entirely:

```cpp
consteval auto operator""_bit(unsigned long long i)
{
    return static_cast<decltype(i)>(1) << i;
}
```

If runtime use is ever intentionally needed, add a guard:

```cpp
constexpr auto operator""_bit(unsigned long long i)
{
    return i < 64 ? static_cast<decltype(i)>(1) << i : 0ULL;
}
```

### F-10 — `set_busbar_temperature_threshold` silently substitutes 125 °C for out-of-range input *(Tier G — latent, dead code in all current builds)*

**Status**: dead code. `BusBarTempSensorNum = 0` in all known platform configs (`p3957_cxx`, `testrunner`, `mcxn547helloworld`). The `if constexpr (nv::ipc::voltage_monitor_config::BusBarTempSensorNum > 0)` gate at `nsm_type_3.cpp:447` compiles away the entire NTC-lookup body; `set_busbar_temperature_threshold` is an unconditional `return Ccode::Success` in every current production build. The bug would activate only if a future platform sets `BusBarTempSensorNum > 0`.

**File**: `src/nv/mctp/nsm_type_3.cpp:445–491`

```cpp
uint32_t resistanceOhm = volt_mon::ntc_temperature_to_resistance(tempCelsius);
if (resistanceOhm == 0) {
    // Invalid temperature, use default max temp (125°C)   <-- silent substitution
    resistanceOhm = volt_mon::ntc_temperature_to_resistance(volt_mon::NtcTempMax);
    // falls through — returns Ccode::Success
}
```

`request.threshold` is a `uint8_t` (0–255) cast to `int16_t`. Values 126–255 exceed `NtcTempMax (125)`, so `ntc_temperature_to_resistance` returns 0 (out-of-range sentinel). The code silently substitutes 125 °C and returns `Ccode::Success` instead of an error, making the threshold-set appear to succeed when it applied a clamped value the caller did not request.

**What ESBMC proved** (`nsm_type3_f10_neg`, VERIFICATION FAILED):

The harness **directly compiles `src/nv/volt_mon/ntc_table.cpp`** (production code — 166-entry real NTC lookup table, not a model). ESBMC traces through the actual `ntc_temperature_to_resistance` implementation. Nondet `threshold` constrained to the out-of-range region (`tempCelsius > NtcTempMax`); harness confirms the real NTC function returns 0 on this path, then asserts the function must not return `Ccode::Success`. CEX: `threshold = 255` → `ntc_temperature_to_resistance(255) = 0` → function returns `Success`.

**Rigor note**: `ntc_table.cpp` is compiled from production source without modification; ESBMC traces the real 166-entry NTC lookup. The `set_busbar_temperature_threshold` logic is inlined verbatim from `nsm_type_3.cpp:445–491` (Tier 1 — not F-1 level). Compiling `nsm_type_3.cpp` directly would require stubs for its transitive hardware headers (`nv/volt_mon/busbar_temp.h`, `nv/volt_mon/leak_detect.h`, `nv/i2c/emc1812.h`, etc.) and a config with `BusBarTempSensorNum > 0` to make the bug path live — a system-harness effort comparable to F-6/F-7. Not pursued because the bug path is dead code in all current builds.

**Config-level note**: every known platform config sets `BusBarTempSensorNum = 0` (`p3957_cxx/config.h:1405`, `testrunner/config.h:668`, `mcxn547helloworld/config.h:1106`), compiling away the entire `if constexpr` block. The harness strips this gate and exercises only the inner body, proving the substitution bug for any future config that enables busbar sensors.

**Runtime confirmation**: sanitizer run with `threshold = 254` → assertion `result != Ccode::Success` fires. `ctest/f10/`.

**Recommendation**: return an error instead of silently substituting:
```cpp
if (resistanceOhm == 0) {
    nv::warn("%s() out-of-range threshold %d°C\n", __func__, tempCelsius);
    return Ccode::ErrorInvalidData;
}
```

---

### DCD GPIO handlers — structural safety proof

**Files**: `src/nv/mctp/nsm.cpp:3389` (`on_dcd_get_gpio`), `nsm.cpp:3482` (`on_dcd_set_gpio`)

Both handlers apply an early-exit guard:

```cpp
if ((offset + length) > nv::ipc::GpioNum) {
    fill_error_packet(Ccode::ErrorInvalidData, rx, tx);
    return;
}
```

The subsequent loop iterates `i ∈ [0, length)` and accesses:

- `GpioSetup.at(gpio_index)` where `gpio_index = offset + i`
- `gpio_resp.gpio.at(byte_index)` where `byte_index = i / 8`

**Structural invariant**: after the guard, `offset + length ≤ GpioNum`, so
`gpio_index < GpioNum = GpioSetup.size()`. `GpioBytes = (GpioNum+7)/8`, so
`byte_index = i/8 < GpioBytes = gpio_resp.gpio.size()`. Both `.at()` calls are
provably in bounds for any valid `(offset, length)` pair.

**What ESBMC proved** (`nsm_gpio_safety`, `GpioNum=66` (p3957_cxx production value),
`--unwind 67`, VERIFICATION SUCCESSFUL, 536 VCC):

All 536 verification conditions discharged. No counterexample exists for any
`offset`, `length` pair that passes the guard, including the discover-all
special case (`offset=0, length=0 → length=GpioNum`).

**Conclusion**: the DCD GPIO handlers are structurally safe. No finding.

---

### DCD event-bitmask write handlers — structural safety proof

**Files**: `src/nv/mctp/nsm.cpp:983` (`on_dcd_set_current_event_srcs`), `nsm.cpp:1096` (`on_dcd_configure_event_ack`)

Sibling check to F-15 / F-16: those findings showed the *read* side
(`is_event_source_enable` / `is_event_ack_enable`) computes
`bitmask.at(event_id / 8)` on a size-8 array without a bounds check, so
`event_id ≥ 64` reaches an OOB index. The *write* side under the same
data structures uses

```cpp
log_nvmsg_event_bitmask.at(static_cast<uint8_t>(msg_with_bitmask.nv_msg_type)) = false;
```

inside an `if (nv_msg_type == DCD) { ... } else if (nv_msg_type == Firmware) { ... }`
guard. `log_nvmsg_event_bitmask` has size `NvMctpSupportedNum = 32`; the
two enumerator values that reach the write are 0 (DCD) and 6 (Firmware),
both well within bounds. The accompanying inner loops iterate
`i ∈ [0, NvMctpEventSupportedNum=8)` over size-8 `type{0,6}_event_*_bitmask`
and `SupType{0,6}Event` arrays.

**What ESBMC proved** (`nsm_dcd_event_handlers`, `--unwind 10`,
VERIFICATION SUCCESSFUL, 691 VCC):

Both handler bodies, executed under nondet `nv_msg_type` and nondet 8-byte
`bitmask`, discharge all 691 verification conditions. No memory-safety,
overflow, NaN, or unsigned-overflow violation is reachable.

**Conclusion**: the asymmetric-guard pattern that bit F-15 / F-16 does not
apply to the write side — `nv_msg_type` is value-checked before use as an
array index, and the index space {0, 6} is contained in the size-32 array.
No finding.

---

### Items checked, no defects

- Packed-struct alignment access in `Packet::to_span()` and `Packet::from()`
  (ESBMC reports the standard 6 packed-struct alignment warnings; these are
  expected for a `[[gnu::packed]]` MCTP wire format and not bugs).
- All `nv::fixed_point` conversion functions are total over their declared
  input ranges; no overflow under the documented preconditions.

### Retracted findings

| ID | Original claim | Retraction reason | ESBMC result |
|---|---|---|---|
| F-2 | `align_to()` intermediate unsigned overflow | Unsigned wrap is benign; `(v + a) - 1` and `v + (a - 1)` are identical modulo 2³² — the intermediate wrap is cancelled by the subsequent mask. | VERIFICATION SUCCESSFUL (equivalence harness + 5 ctests) |
| F-3 | `buf_to_u32` signed left-shift overflow (`int(byte) << 24`) | C++20 [expr.shift]/2 defines signed left-shift for all inputs; no UB. Structural equivalence to the parenthesised form proven by ESBMC. Triggered esbmc#4201, fixed by [#4211](https://github.com/esbmc/esbmc/pull/4211). | VERIFICATION SUCCESSFUL |
| Former F-6 | `buffer_to_uint32` cast-after-shift (`buffer[3] << Byte3`) | Same C++20 reason as F-3; style/portability issue only. Triggered esbmc#4240, fixed by [#4241](https://github.com/esbmc/esbmc/pull/4241). | VERIFICATION SUCCESSFUL |
| F-9 | Shift in `decode_6bit_ascii` | ESBMC returned VERIFICATION SUCCESSFUL. Shift is well-defined under C++20. | VERIFICATION SUCCESSFUL |
| F-12 | `decode_6bit_ascii` output bounds (`decode_6bit_ascii` returning values outside [0x20, 0x5F]) | ESBMC returned VERIFICATION SUCCESSFUL; mask correctly bounds output to [0x20, 0x5F] for all inputs. | VERIFICATION SUCCESSFUL |
| F-14 | `Pca9555::i2c_write` bare `return` on Input command drops bytes | `CommandRegister` is fixed per call (`cmd_byte / 2`); `case Input` body is empty; `return` and `break` are observably equivalent. Production `pca9555.cpp` compiled by ESBMC confirms no state change. Code-quality note only. | VERIFICATION SUCCESSFUL (405 VCC) |

Full analysis for each retraction is in [NOTES.md](NOTES.md).

## Tooling-level findings (ESBMC bugs)

### Active workarounds

One workaround remains in the tree for an issue whose fix has not yet fully
propagated to the ESBMC binary in use:

| Issue | Description | Workaround in tree |
|---|---|---|
| [#4281](https://github.com/esbmc/esbmc/issues/4281) | `[[gnu::packed]]` bitfield member-initialiser in a constructor triggers a false-positive bounds/alignment check (`dereference failure: Access to object out of bounds` / `Misaligned access to struct field`). Reproduced on `NsmDevCfgErrorInjectionModeResponse()` — a packed struct with a `uint8_t` field followed by two bitfield members. | `--no-align-check` on `nsm_f6_system` and `nsm_f7_system` targets. Repro: `esbmc_bug_repros/packed_bitfield_ctor_bounds_fp.cpp`. |

### Closed issues

The following ESBMC issues were surfaced during this work and are now fully
resolved with no remaining workarounds in the tree:

[#4180](https://github.com/esbmc/esbmc/issues/4180) (umbrella; split into #4183/#4184),
[#4190](https://github.com/esbmc/esbmc/issues/4190) (fixed by [#4192](https://github.com/esbmc/esbmc/pull/4192) + [#4194](https://github.com/esbmc/esbmc/pull/4194) + [#4244](https://github.com/esbmc/esbmc/pull/4244) — `<bit>`, `<span>`, `<type_traits>`, and `<array>` aggregate),
[#4182](https://github.com/esbmc/esbmc/issues/4182) (fixed by [#4187](https://github.com/esbmc/esbmc/pull/4187)),
[#4183](https://github.com/esbmc/esbmc/issues/4183) (fixed by [#4188](https://github.com/esbmc/esbmc/pull/4188)),
[#4195](https://github.com/esbmc/esbmc/issues/4195) (fixed by [#4204](https://github.com/esbmc/esbmc/pull/4204)),
[#4201](https://github.com/esbmc/esbmc/issues/4201) (resolved via [#4211](https://github.com/esbmc/esbmc/pull/4211)),
[#4213](https://github.com/esbmc/esbmc/pull/4213) (merged; `underlying_type` added to bundled `<type_traits>`),
[#4214](https://github.com/esbmc/esbmc/issues/4214) (fixed by [#4215](https://github.com/esbmc/esbmc/pull/4215)),
[#4216](https://github.com/esbmc/esbmc/issues/4216) (closed by #4217; residual crashes fixed by [#4233](https://github.com/esbmc/esbmc/pull/4233) and [#4235](https://github.com/esbmc/esbmc/pull/4235)),
[#4237](https://github.com/esbmc/esbmc/issues/4237) (fixed by [#4238](https://github.com/esbmc/esbmc/pull/4238)),
[#4240](https://github.com/esbmc/esbmc/issues/4240) (fixed by [#4241](https://github.com/esbmc/esbmc/pull/4241)),
[#4243](https://github.com/esbmc/esbmc/issues/4243) (fixed by [#4244](https://github.com/esbmc/esbmc/pull/4244)),
[#4245](https://github.com/esbmc/esbmc/issues/4245) (fixed by [#4246](https://github.com/esbmc/esbmc/pull/4246)),
[#4247](https://github.com/esbmc/esbmc/issues/4247) (fixed — bundled `<bit>` pointer overload now accepts const From; `stubs/bit` removed),
[#4248](https://github.com/esbmc/esbmc/issues/4248) (fixed — bundled `<span>` now transitively includes `<bit>`),
[#4249](https://github.com/esbmc/esbmc/issues/4249) (fixed — bundled `<span>` relative `#include "array"` replaced; `stubs/span` removed),
[#4251](https://github.com/esbmc/esbmc/issues/4251) (fixed — bundled `<algorithm>` now provides `std::clamp`; `stubs/algorithm` removed),
[#4264](https://github.com/esbmc/esbmc/issues/4264) (fixed — `chrono::duration::max()` now compiles correctly in ESBMC's bundled `<chrono>`),
[#4267](https://github.com/esbmc/esbmc/issues/4267) (partially fixed — general packed-struct alignment suppression landed; residual packed-bitfield-constructor case re-filed as [#4281](https://github.com/esbmc/esbmc/issues/4281)),
[#4269](https://github.com/esbmc/esbmc/issues/4269) (fixed — bundled `<array>` now exposes `constexpr operator[]` and `at()`; `stubs/array` removed),
[#4270](https://github.com/esbmc/esbmc/issues/4270) (fixed — bundled `<span>` relative `#include "array"` path corrected; `stubs/span` removed),
[#4271](https://github.com/esbmc/esbmc/issues/4271) (fixed — `using Base::Base` (ConstructorUsingShadow) now handled correctly by ESBMC's Clang frontend),
[#4272](https://github.com/esbmc/esbmc/issues/4272) (fixed — `std::tuple` is now a literal type in ESBMC's bundled `<tuple>`; `inline const` workarounds in `stubs/nv/mctp/nsm_type_4.h` and `stubs/nsm_f6_config.h` removed),
[#2789](https://github.com/esbmc/esbmc/issues/2789) (fixed by [#4242](https://github.com/esbmc/esbmc/pull/4242)).

## What was deferred and why

- **FreeRTOS-backed code** (`src/nv/ipc/queue.cpp`, `src/nv/ipc/event.cpp`,
  `src/nv/ipc/timer.cpp`) — the actual logic is in
  `src/sys/x86/sys/ipc/queue.cpp`, which delegates to `xQueueSendToBack`,
  `xQueueReceive`, etc. Modeling FreeRTOS queues is a project of its own;
  out of scope for the initial sweep.
- **Ada units** (`*.ads`, `*.adb`) — ESBMC has no Ada frontend. These will
  need to be stubbed at the C ABI boundary if they're ever in scope.

## Suggested next steps

1. **Fix F-1** — add the `interface >= UsEnd` guard to `set_cur_eid()` (or
   tighten `Validator::validate()` to reject `>= UsEnd`). Confirmed abort path
   for any Control SetEpId Request with `priv.packet_interface ∈ [2, 17]`.
2. **Fix F-6, F-7, F-8, F-13** — each section above contains a specific
   one- or two-line recommendation. F-8 is latent (existing call site has a
   guard); the other three are directly reachable. **F-10** is dead code in all
   current builds (`BusBarTempSensorNum = 0` everywhere) — apply the fix
   preemptively so the correct `ErrorInvalidData` path is in place before any
   future platform enables busbar sensors.
3. **Fix F-5, F-15, and F-16** — all three stem from the same asymmetric-guard
   pattern. Add the `byte_index >= bitmask.size()` guard to `set_bit` /
   `unset_bit` (F-5), and an equivalent `ByteIndex >= bitmask.size()` guard to
   both `is_event_source_enable` (F-15) and `is_event_ack_enable` (F-16).
   Low urgency: no current call site passes a value ≥ 64; the risk is from
   future callers only.
4. **Continue fresh sweep of unverified packet handlers** — search for the
   validator-gap pattern that made F-1 exploitable: any handler that takes a
   packet-provided integer and uses it as an array index (`.at(x)` or `arr[x]`)
   or a shift count (`1 << x`) without a prior bounds check. Priority targets:
   `nsm.cpp` dispatch handlers not yet covered (event subscription,
   GetSupportedDeviceModes), and any `nsm_type_*.cpp` handler outside the
   already-verified field-validator subset.
5. **Stand up a CI hook** — run `make all` on every PR; verification must
   stay green and any failure must be triaged before merge.

## Reproducing

```sh
cd verification
make all                    # all Phase 1 targets (includes nsm_type3, nsm_bitmask, nsm_type5_validate)
make mctp_packet_func       # Phase 2 (k-induction)
make mctp_router_func
make fixed_point_func
make nsm_type3_func         # nsm_type3 availability contracts (k=9)
make nsm_bitmask_func       # bitmask contracts (k=1)
make nsm_type5_validate_func  # nsm_type5 field-validator contracts (k=1)
make ntc_table_func         # NTC table contracts: exact lookup, range clamping, round-trip (k=9)
make pwr_smooth_params_func # OverrideParam round-trips + is_valid_param_id characterisation (k=1)
make fru_utils_func         # checksum contract + decode_6bit_ascii output-range invariant (k=9)
make soc_sma_filter_func    # SocSmaFilterCh steady-state identity + output-bounded (k=1)
make debug_telemetry_sma_func  # DebugTelemetrySmaCh index-bounded + output-nonneg (k=1)
make pca9555_func           # Pca9555 direction constraint + masked input update + interrupt + output (k=2)
make emc1812_func           # Emc1812 int8_t↔uint8_t cast round-trip identity (k=1)
make tmp1075_func           # Tmp1075 12-bit encoding round-trip identity (k=1)
make tmp461_func            # Tmp461 int8_t↔uint8_t cast round-trip identity (k=1)
make mctp_packet_neg        # negative tests (expect VERIFICATION FAILED)
make mctp_router_neg
make mctp_dispatch          # F-1 reachability proof (expect VERIFICATION FAILED)
make nsm_bitmask_neg        # F-5: set_bit OOB on 8-element array (expect VERIFICATION FAILED)
make nsm_type5_f6_neg       # F-6: unchecked mode byte stored (expect VERIFICATION FAILED)
make nsm_type5_f7_neg       # F-7: no rollback after validation failure (expect VERIFICATION FAILED)
make nsm_f7_system          # F-7 system-level: real nsm_type_5.cpp, validator always true (expect VERIFICATION SUCCESSFUL)
make nsm_type5_f8_neg       # F-8: gpio ei_entries[16] OOB (expect VERIFICATION FAILED)
make nsm_type3_f10_neg      # F-10: silent 125°C substitution — real ntc_table.cpp (expect VERIFICATION FAILED)
make debug_telemetry_f13_neg     # F-13: negative percent wrap to uint8 (expect VERIFICATION FAILED)
make debug_telemetry_f13_system  # F-13 reachability proof via PowerManager::run_iteration() (expect VERIFICATION SUCCESSFUL)
make pca9555_f14_neg             # F-14: retracted — production pca9555.cpp (expect VERIFICATION SUCCESSFUL)
make nsm_event_source_f15_neg   # F-15: is_event_source_enable OOB on event_id≥64 (expect VERIFICATION FAILED)
make nsm_event_ack_f16_neg      # F-16: is_event_ack_enable OOB on event_id≥64 (expect VERIFICATION FAILED)
make nsm_gpio_safety            # DCD GPIO structural safety proof (expect VERIFICATION SUCCESSFUL)
make nsm_dcd_event_handlers     # on_dcd_set_current_event_srcs / on_dcd_configure_event_ack structural safety (expect VERIFICATION SUCCESSFUL)
```

ESBMC 8.2.0 on `$PATH`, or pass `ESBMC=/path/to/esbmc make ...`.

### CTest-generated executable validators

```sh
# F-2 retraction: empirical equivalence of buggy and fixed forms.
cd verification/ctest/f2
esbmc --std c++20 --branch-coverage --generate-ctest-testcase align_equiv.cpp
mkdir -p build && cd build && cmake .. && cmake --build . && ctest

# F-1 failure mode: std::array::at(OOB) under production flags.
cd verification/ctest/f1
esbmc --std c++20 -I../../stubs --branch-coverage --generate-ctest-testcase array_oob.cpp
mkdir -p build && cd build && cmake .. && cmake --build .
./test_case_1   # expect SIGABRT (exit 134)
```
