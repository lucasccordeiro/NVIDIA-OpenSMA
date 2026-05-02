# OpenSMA ESBMC Verification — Initial Report

**Date**: 2026-04-25 (updated 2026-05-02)
**Tool**: ESBMC 8.2.0 (aarch64-macos)
**Scope**: bounded model checking of selected modules in
[NVIDIA/OpenSMA](https://github.com/NVIDIA/OpenSMA)

## TL;DR

Twenty-two modules verified end-to-end against language-level safety properties
(pointer/bounds/overflow/div-by-zero/memory-leak) and against module-specific
functional contracts via k-induction. **One vulnerability formally proven
reachable** (F-1) via `mctp_dispatch` — ESBMC finds a counterexample where a
Control SetEpId Request with a gap interface triggers `set_cur_eid()` to
OOB-index the 2-entry `cur_eid` array. **Five additional security findings
formally confirmed** by ESBMC (VERIFICATION FAILED on dedicated negative
harnesses) and independently reproduced by native execution under address /
undefined-behaviour sanitizers: **F-6** (unchecked mode byte in
`on_dev_cfg_set_errorInjectionMode`), **F-7** (no rollback after
`PortRecoveryPayload` validation failure), **F-8** (OOB in
`validateGpioSpoofingErrorInjectionPayload`), **F-10** (silent 125 °C
substitution in `set_busbar_temperature_threshold`), **F-13** (negative percent
wrap in `DebugTelemetrySmaCh`). Three findings retracted after ESBMC returned
VERIFICATION SUCCESSFUL (**F-9** — shift defined under C++20, **F-12** — mask
correctly bounds 6-bit ASCII decode output, **F-14** — bare `return` and `break`
observably equivalent for Input-register writes in `Pca9555::i2c_write`). Two
initially-claimed findings (F-2, F-3) **retracted on review**. Two
confirmed latent-UB findings: **F-4** in `literals.h::operator""_bit`
(shift-count ≥ 64) and **F-5** in `nsm_msg_bitmask.h::set_bit` / `unset_bit`
on the 8-element event bitmask (index ≥ 64 reaches `std::array::at` OOB).
Neither F-4 nor F-5 has a dangerous current call site, but F-5 lacks the
runtime guard that sibling operations carry. Several ESBMC C++-frontend bugs filed against
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
| NSM type 3 sensor availability | `src/nv/mctp/nsm_type_3.cpp` (`is_temp_sensor_available`, `is_power_sensor_available`, `is_voltage_sensor_available`) | ✅ 37 VCC | ✅ k=9 (membership iff, busbar-unavailable exclusion, voltage always-false) | ✅ **F-10** CEX: `threshold=254` → Success returned for out-of-range temperature |
| Telemetry sensor-ID lookup + LE deserialiser | `src/nv/telemetry/utils.h` (`getTelemIdFromTempSensorId`, `getTelemIdFromPowerSensorId`, `buffer_to_uint32`) | ✅ 80 VCC | ✅ k=11 (mapping iff, MaxItem for non-members, LE byte-order contract) | — |
| SPI byte-buffer (de)serialisation | `src/nv/spi/utils.{h,cpp}` (`buf_to_u{16,32}`, `u{16,32}_to_buf`) | ✅ | ✅ k=9 (round-trip + big-endian + OOB-no-write) | — |
| I2C CRC-8 helpers | `src/nv/i2c/helper.cpp` (`crc8`) | ✅ | ✅ k=5 (incrementality + init-zero invariant) | — |
| User-defined integer literals | `src/nv/common/literals.h` (`_u8`/`_u16`/`_u32`/`_i8`/`_i16`/`_i32`/`_bits_sizeof`/`_bit`) | ✅ | ✅ k=1 (mask agreement, signed/unsigned truncation parity, `bits/8`, `1ULL << i`) | ✅ CEX on `_bit(i≥64)` via `--ub-shift-check` — **F-4** |
| NSM bitmask operations | `src/nv/mctp/nsm_msg_bitmask.h` (`set_bit`/`unset_bit`/`get_bit`/`is_bit_set`) | ✅ 75 VCC | ✅ k=1 (set→get non-zero; unset→get zero; is_bit_set iff get_bit≠0) | ✅ CEX on `set_bit`/`unset_bit(arr8, pos≥64)` — **F-5** |
| NSM type 5 field validators | `src/nv/mctp/nsm_type_5.cpp` (`validateFatalErrorInjectionPayload`, `validateDeviceIndex{GpuDegradeMode,PowerSupply}`, `validateAction{GpuDegradeMode}`, `validateModePowerSupply`) | ✅ 14 VCC | ✅ k=1 (exact characterisation: accepted iff bitmask∈{0,1,2}, index/mode in documented ranges) | ✅ **F-6** CEX: `mode=0xFF` stored; **F-7** CEX: dirty `portRecoveryResp` on validation failure; **F-8** CEX: `gpio_ei_entries[16]` OOB |
| NTC thermistor table | `src/nv/volt_mon/ntc_table.{h,cpp}` (`ntc_resistance_to_temperature`, `ntc_voltage_to_temperature`, `ntc_adc_to_temperature`, `ntc_temperature_to_resistance`, `ntc_temp_to_adc_value`) | ✅ 227 VCC | ✅ k=9 (exact table lookup, range clamping, round-trip identity) | — |
| Power-smoothing params | `src/nv/soc_pwr_smoothing/presets.{h,cpp}` (`OverrideParam::to_uint32`, `::from_uint32`, `is_valid_param_id`) | ✅ 72 VCC | ✅ k=1 (round-trip pack↔unpack identity, param-id exact characterisation) | — |
| FRU utilities | `src/nv/fru/fru.cpp` (`verify_checksum`, `decode_6bit_ascii`) | ✅ 76 VCC | ✅ k=9 (checksum true iff sum≡0 mod 256, decode output ∈ [0x20, 0x5F]) | — |
| SoC SMA filter | `src/nv/soc_pwr_smoothing/soc_sma_filter_ch.h` (`SocSmaFilterCh::evaluate` — 4-sample sliding-window SMA over SFXP22_10) | ✅ 504 VCC | ✅ k=1 (steady-state: 4 equal inputs → output == input; output ∈ [0, input]) | — |
| Debug telemetry SMA | `src/nv/soc_pwr_smoothing/debug_telemetry_sma_ch.h` (`DebugTelemetrySmaCh::evaluate` — 256-sample SMA; UFXP8_0 buffer; percent ∈ [0%, 150%]) | ✅ 261 VCC | ✅ k=1 (index bounded ∈ [0, 255] by bitwise-AND; output non-negative from zero state) | ✅ **F-13** CEX: `percent=-1024` → `stored=255` (negative wrap) |
| PCA9555 GPIO expander emulator | `src/nv/emulation/pca9555.{h,cpp}` (`Pca9555` — 16-bit I2C GPIO expander; direction/input/output/inversion registers + interrupt-on-change logic) | ✅ 1292 VCC | ✅ k=2 (direction constraint with precondition req_in∩req_out=∅; input_update_masked; interrupt_default; output_propagation) | ✅ VERIFICATION SUCCESSFUL (405 VCC, production `pca9555.cpp`): output state unchanged after Input-register write — **F-14 retracted** (bare `return` and `break` observably equivalent; code-quality note) |
| EMC1812 temperature sensor driver | `src/nv/i2c/emc1812.{h,cpp}` (`Emc1812` — EMC1812 temp sensor driver; all public methods with nondet I2C stubs; `int8_t↔uint8_t` threshold cast round-trip verified for all six set/get pairs) | ✅ 52 VCC | ✅ k=1 (cast_roundtrip: `static_cast<int8_t>(static_cast<uint8_t>(t)) == t` for all `int8_t t`; threshold_symmetry: all four pairs) | — |
| TMP1075 temperature sensor driver | `src/nv/i2c/tmp1075.{h,cpp}` (`Tmp1075` — 12-bit two's-complement temperature encoding: `int8_t → <<4 → int16_t → uint16_t → >>4 → int8_t` round-trip; `get_device_id`; `set/get_{low,high}_limit`) | ✅ 33 VCC | ✅ k=1 (12bit_roundtrip: `static_cast<int8_t>(static_cast<int16_t>(static_cast<uint16_t>(static_cast<int16_t>(t<<4)))>>4) == t` for all `int8_t t`; temp_read_cast well-defined) | — |
| TMP461 temperature sensor driver | `src/nv/i2c/tmp461.{h,cpp}` (`Tmp461` / NCT72 — `int8_t↔uint8_t` threshold cast round-trip for four alert/therm set/get pairs; `get_configuration`) | ✅ 57 VCC | ✅ k=1 (cast_roundtrip + threshold_symmetry for all four pairs) | — |

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

### F-1 — `set_cur_eid()` lacks bounds check (reachability formally proven)

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

### F-4 — `operator""_bit` missing precondition guard on shift count

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

### F-5 — `set_bit` / `unset_bit` missing bounds guard on the 8-element event bitmask

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

### F-6 — `on_dev_cfg_set_errorInjectionMode` stores unchecked mode byte

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

**Rigor note**: the harness inlines the production logic verbatim (confirmed line-for-line against `nsm_type_5.cpp:777–803`). Full compilation of `nsm_type_5.cpp` with ESBMC is blocked by esbmc#4245 (`<optional>` and `<chrono>` missing from ESBMC's bundled C++ library) and by hardware-specific headers (`mbedtls/ctr_drbg.h`, `sys/adc/adc.h`).

**Runtime confirmation**: sanitizer run (`-fsanitize=address,undefined`) with `request_mode = 0xFF` triggers `assert(mode == Disable || mode == Enable)` → SIGABRT. `ctest/f6/`.

**Recommendation**: add a range check before the assignment:
```cpp
if (nrx.data[0] != Disable && nrx.data[0] != Enable)
    return Ccode::ErrorInvalidData;
type5_data.errorInjectionModeResponse.mode = nrx.data[0];
```

---

### F-7 — `on_dev_cfg_set_portRecoveryErrorInjection` writes before validating (no rollback)

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

**Runtime confirmation**: sanitizer run with `incoming.offset = 42` and validator forced to return false → assertion fires. `ctest/f7/`.

**Recommendation**: validate before writing, or save and restore on failure:
```cpp
// Option A: validate-then-write
if (!validatePortRecoveryErrorInjectionPayload(...))
    return Ccode::ErrorInvalidData;
memcpy(&portRecoveryEIPayload, nrx.data, sizeof(portRecoveryEIPayload));
```

---

### F-8 — `validateGpioSpoofingErrorInjectionPayload` lacks bounds check on `ei_gpio_entries` (latent)

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

### F-10 — `set_busbar_temperature_threshold` silently substitutes 125 °C for out-of-range input

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

**Rigor note (F-1 style)**: `ntc_table.cpp` is compiled from production source without modification. The `set_busbar_temperature_threshold` logic is inlined verbatim (confirmed line-for-line against `nsm_type_3.cpp:445–491`); only the hardware `BusbarTemp` singleton (ADC interaction) is stubbed. Full compilation of `nsm_type_3.cpp` with ESBMC is blocked by esbmc#4245 and hardware headers (`sys/adc/adc.h`).

**Config-level note**: the testrunner `config.h` sets `BusBarTempSensorNum = 0`, which compiles away the entire if-constexpr block and makes this code path unreachable in the testrunner build. On production hardware `BusBarTempSensorNum > 0` and the path is live.

**Runtime confirmation**: sanitizer run with `threshold = 254` → assertion `result != Ccode::Success` fires. `ctest/f10/`.

**Recommendation**: return an error instead of silently substituting:
```cpp
if (resistanceOhm == 0) {
    nv::warn("%s() out-of-range threshold %d°C\n", __func__, tempCelsius);
    return Ccode::ErrorInvalidData;
}
```

---

### F-13 — `DebugTelemetrySmaCh::evaluate` wraps negative percent to unsigned

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

---

### F-14 — RETRACTED: `Pca9555::i2c_write` bare `return` on Input command (code-quality observation)

**File**: `src/nv/emulation/pca9555.cpp:142–175`

```cpp
CommandRegister = cmd_byte / 2;           // fixed for all iterations
for (uint8_t i = start_index; i < data_length; i++) {
    switch (CommandRegister) {
        case Input:
            return;    // bare return — but Input case has no body
        case Output: ...
    }
}
```

**Initially filed** based on an inline-model harness (not compiling the production source) that asserted `completed == expected` iterations and found a CEX. That analysis was incorrect.

**Why it was wrong**: `CommandRegister` is computed *once* before the loop as `cmd_byte / 2` and does not change across iterations. For any Input-register write (`cmd_byte ∈ {0, 1}`), `CommandRegister == Input` on every iteration. The `case Input` body is empty — there are no state mutations in that case. Therefore the bare `return` and a `break` produce **identical observable state**: `_gpio_output`, `_gpio_direction`, and `_gpio_inversion` are all unchanged either way. The commented-out `nv::warn("trying to write to PCA9555 input pin")` confirms the author treated this as an intentional caller-error path.

**What the upgraded harness confirmed** (`pca9555_f14_neg`, **VERIFICATION SUCCESSFUL — 405 VCC**):

The harness was rewritten to call the **actual production `Pca9555::i2c_write()`** from `pca9555.cpp` compiled by ESBMC (same rigor as F-1). It snapshots `output_before`, calls `dev.i2c_write(buf, 3)` with `buf[0]=0x00` (Input register) and nondet data bytes, then asserts `output_after == output_before`. ESBMC explored 405 verification conditions and found no violation. The production code path through `pca9555.cpp:142` was confirmed reachable and safe.

**Classification**: code-quality / documentation issue. The bare `return` is not wrong — it is observably equivalent to `break` for Input registers — but `break` would communicate intent more clearly, and the commented-out warn() line suggests the intent was never documented.

**Recommendation** (style only, not a security fix): replace `return` with `break` and uncomment or add a brief comment explaining that Input registers are hardware-read-only and writes are silently ignored.

---

### Former F-6 (retracted) — `buffer_to_uint32` misplaced cast

**File**: `src/nv/telemetry/utils.cpp:31`

```cpp
| (static_cast<uint32_t>(buffer[3] << Byte3));  // cast after shift — initially flagged
```

The cast is applied after the shift rather than before. Initially filed as "signed-shift UB for `buffer[3] >= 0x80` under C++20." **Retracted**: C++20 P0907R4/P1236R1 makes signed left-shift fully defined for all inputs — the result is the unique value congruent to `E1 × 2^E2` modulo `2^N` — removing both the negative-E1 and result-overflow UB clauses that existed in C++17. ESBMC's bitvector model was already correct; `--overflow-check` rightly generates no VCC for this expression under `--std c++20`.

This is the same standard-conformance question as the retracted F-3 (`buf_to_u32`). The REPORT.md entry for F-3 already quoted this rule correctly ("Under C++20+ `[expr.shift]/2`, signed left-shift `E1 << E2` is well-defined"); F-6 should have been retracted on the same grounds.

The code pattern is still a **style/portability issue**: bytes 0–2 cast before shifting (`static_cast<uint32_t>(buffer[N]) << Byte`) while byte 3 casts after. The recommended fix (move the cast before the shift) makes the intent uniform and correct even under C++17:
```cpp
| (static_cast<uint32_t>(buffer[3]) << Byte3)
```

**ESBMC issue filed as a consequence** — the misanalysis led to filing [esbmc#4240](https://github.com/esbmc/esbmc/issues/4240) ("ESBMC misses signed shl overflow UB under C++20"). That framing was wrong (no UB to miss), but esbmc#4240 uncovered a real ESBMC defect in the opposite direction: under `--std c++20`, `--overflow-check` and `--ub-shift-check` were still generating false-positive VCCs for signed shl (E1 with unknown sign, and E1 < 0 respectively). Fixed by [#4241](https://github.com/esbmc/esbmc/pull/4241). Repro retained at `esbmc_bug_repros/signed_shift_result_overflow.cpp`.

**What ESBMC verified** (`telemetry`, 80 VCC Phase 1 with `--ub-shift-check`, k=11 Phase 2): both the buggy and fixed forms satisfy the little-endian decoding contract across all 4-byte inputs (the two expressions are structurally equivalent under C++20 semantics).

### F-3 — RETRACTED (was: `buf_to_u32` signed shift overflow)

ESBMC's `--overflow-check` flagged `buf[start_idx] << ByteShift3` (i.e. `int(byte) << 24`) as an arithmetic-overflow violation when `byte >= 0x80`. Investigated:

- Production builds with `-std=c++23`. Under C++20+ ([expr.shift]/2), signed left-shift `E1 << E2` is well-defined: the value is the unique result congruent to `E1 × 2^E2` modulo `2^N` where `N` is the width of the result type. For `int(128) << 24`, that's `INT_MIN`; the surrounding `static_cast<uint32_t>(...)` then recovers the correct `0x80000000` bit pattern. **No UB.**
- Empirically validated: ESBMC's BMC proves `prod_form(b0, b1, b2, b3) == fixed_form(b0, b1, b2, b3)` for all four input bytes (0 VCCs after simplification — equivalence is structural). See `verification/ctest/f3/`.

Not a defect in OpenSMA. The standard-conformance gap that surfaced this — the default `--overflow-check` applying pre-C++20 UB rules irrespective of `--std` — was filed as [esbmc/esbmc#4201](https://github.com/esbmc/esbmc/issues/4201). [PR #4203](https://github.com/esbmc/esbmc/pull/4203) was merged then reverted by [#4208](https://github.com/esbmc/esbmc/pull/4208) the same day: the skip condition was too broad. P0907 (merged into C++20) only made signed left-shift modular when `E1` is non-negative; for negative `E1` the shift remains UB, and #4203 would have suppressed that case too. The replacement [PR #4211](https://github.com/esbmc/esbmc/pull/4211) is open with the refined fix: a type-driven non-negativity predicate on `E1` (covers `uint8_t`/`uint16_t`-promoted operands — the OpenSMA case — without symbolic reasoning), `--std c++20+` discrimination via a hand-rolled non-throwing parser bounded to `[20, 50]` so legacy `c++98`/`c++03` spellings stay strict, and 7 CORE regressions covering both halves. Symbolic non-negativity via `--interval-analysis` for arbitrary signed `int` is deferred as layer 2. Until #4211 merges, the spi_utils harness keeps the parenthesisation workaround on mainline ESBMC; the equivalence ctest at `verification/ctest/f3/` is retained as a regression sentinel for the underlying defined-behaviour claim.

### F-2 — RETRACTED (was: `align_to()` overflow)

I initially claimed `align_to` had a real overflow bug. **It does not.**
The retraction is empirically validated via ESBMC's
`--branch-coverage --generate-ctest-testcase` on an equivalence harness
(`verification/ctest/f2/align_equiv.cpp`) that asserts
`align_buggy(v, A) == align_fixed(v, A)` for all inputs. ESBMC's BMC
explored the full input space without finding a counterexample to the
equivalence, and all 5 generated runtime test cases pass at execution time
(`100% tests passed, 0 tests failed`).

For unsigned arithmetic, `(value + alignment) - 1` and
`value + (alignment - 1)` are identically equal modulo 2³² because
addition and subtraction are modular and the operations cancel. Concretely,
for `value = alignment = 0x80000000`:

- `(0x80000000 + 0x80000000) - 1 = 0 - 1 = 0xffffffff (mod 2³²)`
- `0x80000000 + 0x7fffffff = 0xffffffff`

Both then go through `& ~(alignment - 1) = & 0x80000000` → `0x80000000`,
which is the correct aligned result. ESBMC's `--unsigned-overflow-check`
flagged the intermediate wrap on `value + alignment`, but unsigned wrap is
**defined behaviour** in C++ — the flag catches *unintended* wraps for
review, not bugs.

The `make utils_neg` harness still produces a counterexample (the wrap is
real, just benign); the harness is retained as a demonstrator of ESBMC's
strict unsigned-overflow flag, not as a regression sentinel for a defect.
The "fix" applied in `utils_harness.cpp` (parenthesising as
`value + (alignment - 1)`) is a readability/intent improvement that makes
the arithmetic match the guard's expression — adoption is a style choice,
not a correctness one.

**Lesson**: when ESBMC reports an `--unsigned-overflow-check` violation,
verify whether the wrap matters for the function's *output*. A wrap that
is reverted by a subsequent inverse operation is an artefact, not a bug.

### Items checked, no defects

- Packed-struct alignment access in `Packet::to_span()` and `Packet::from()`
  (ESBMC reports the standard 6 packed-struct alignment warnings; these are
  expected for a `[[gnu::packed]]` MCTP wire format and not bugs).
- All `nv::fixed_point` conversion functions are total over their declared
  input ranges; no overflow under the documented preconditions.

## Tooling-level findings (ESBMC bugs)

Five C++ frontend bugs surfaced while building the harnesses. Each has a
freestanding minimal reproducer under `verification/esbmc_bug_repros/`
and is filed upstream. **Every workaround currently in this tree is
backed by an open issue.**

| Issue | Title | State | Workaround in tree |
|---|---|---|---|
| [#4180](https://github.com/esbmc/esbmc/issues/4180) | Original umbrella (array crash + qualified constexpr) | **closed** — split into #4183 (still open) and fixed via #4184 | n/a |
| [#4182](https://github.com/esbmc/esbmc/issues/4182) | `using ns::T;` for class / enum types fails conversion | **fixed** by [#4187](https://github.com/esbmc/esbmc/pull/4187) (merged 2026-04-26) | (workarounds removed; harnesses now use plain `using ns::T;`) |
| [#4183](https://github.com/esbmc/esbmc/issues/4183) | `std::array<T,N>` crashes `gen_vptr_initializations` | **fixed** by [#4188](https://github.com/esbmc/esbmc/pull/4188) (merged 2026-04-26) | (crash gone; `<array>` shim retained for the unrelated aggregate-init divergence — see #4190 below) |
| [#4190](https://github.com/esbmc/esbmc/issues/4190) | bundled libc++ missing `<span>`, `<bit>`, parts of `<type_traits>`; bundled `<array>` is a `class` not an aggregate | partial — [#4194](https://github.com/esbmc/esbmc/pull/4194) bundled `<span>` + most traits; [#4213](https://github.com/esbmc/esbmc/pull/4213) added `std::underlying_type`/`underlying_type_t`; aggregate-`<array>` still missing | thin `<span>` shim (transitive `<bit>` + avoids bundled-`<array>` collision); `<array>` shim retained for aggregate-init |
| [#4191](https://github.com/esbmc/esbmc/issues/4191) | spurious CEX on aliased `*std::bit_cast<T*>(...)` round-trip | fixed by [#4192](https://github.com/esbmc/esbmc/pull/4192) — but the bundled pointer overload uses `reinterpret_cast` (drops const → compile error on `bit_cast<uint8_t*>(this)` in const methods) | thin `<bit>` shim that keeps the pointer aliasing fix and uses C-cast for the pointer specialisation (preserves `std::bit_cast`'s const-agnostic semantics); follow-up commented on #4191 |
| [#4195](https://github.com/esbmc/esbmc/issues/4195) | C++20 `using enum X;` (`UsingEnumDecl`) not handled | fixed by [#4204](https://github.com/esbmc/esbmc/pull/4204) (merged 2026-04-28) | (sed-patch dropped; validator.cpp compiles directly from upstream) |
| [#4201](https://github.com/esbmc/esbmc/issues/4201) | `--overflow-check` flags signed left-shift wrap that is defined under C++20+ | resolved — [#4203](https://github.com/esbmc/esbmc/pull/4203) merged then reverted by [#4208](https://github.com/esbmc/esbmc/pull/4208) (skip too broad); refined replacement [#4211](https://github.com/esbmc/esbmc/pull/4211) **merged** with a type-driven non-negativity predicate on `E1` | spi_utils harness uses production form directly (parenthesisation workaround removed) |
| [#4184](https://github.com/esbmc/esbmc/pull/4184) | `getAsType` guard for namespace-qualified constexpr | merged 2026-04-26 | (workarounds removed) |
| [#4188](https://github.com/esbmc/esbmc/pull/4188) | tag-id mismatch in `annotate_class_method` | merged 2026-04-26 | (crash gone; see #4190 row for the residual `<array>` shim reason) |
| [#4187](https://github.com/esbmc/esbmc/pull/4187) | `UsingType` handling for clang ≥ 22 | merged 2026-04-26 | (workarounds removed) |
| [#4192](https://github.com/esbmc/esbmc/pull/4192) | bundle `<bit>` with pointer-aware `bit_cast` | merged 2026-04-27 | validator Phase 2 ungated; packet overlay deleted; `<bit>` shim retained as thin const-aware override |
| [#4194](https://github.com/esbmc/esbmc/pull/4194) | bundle `<span>` and complete `<type_traits>` | merged 2026-04-27 | `<span>` shim retained as thin replacement (transitive `<bit>` + avoid bundled-`<array>` collision); utils.h still inlined for residual `underlying_type_t` gap |
| [#4203](https://github.com/esbmc/esbmc/pull/4203) | skip signed-shl overflow claim under C++20+ | merged 2026-04-28, **reverted by [#4208](https://github.com/esbmc/esbmc/pull/4208)** the same day (skip condition too broad — would suppress still-UB negative-`E1` case) | spi_utils workaround restored |
| [#4204](https://github.com/esbmc/esbmc/pull/4204) | handle `UsingEnumDecl` (C++20 `using enum`) | merged 2026-04-28 | (sed-patch dropped; validator.cpp compiles directly) |
| [#4211](https://github.com/esbmc/esbmc/pull/4211) | replacement for #4203: skip signed-shl overflow only when `E1` is provably non-negative (type-driven predicate); standard-aware via `--std c++20+` parsing; legacy spellings (`98`, `03`) and pre-C++20 unaffected | **merged** (7 CORE regressions, paired with the OpenSMA harness restoration) | spi_utils parenthesisation workaround removed |
| [#4213](https://github.com/esbmc/esbmc/pull/4213) | add `std::underlying_type` and `underlying_type_t` to bundled `<type_traits>` (SFINAE-guarded via `__underlying_type(T)` builtin; `::type` only present for enum types) | **merged** (2 CORE regressions: positive and negative) | `utils.h` workaround removed; harness now includes production header directly |
| [#4214](https://github.com/esbmc/esbmc/issues/4214) | `platforms::Control` default-construction triggers assertion `new_comp.size() == ops.size()` in `clang_c_adjust_expr.cpp:158`; ESBMC aborts during GOTO program creation | **fixed** by [#4215](https://github.com/esbmc/esbmc/pull/4215) (merged 2026-04-29) | (workaround note updated; `ctrl{}` now constructs cleanly) |
| [#4216](https://github.com/esbmc/esbmc/issues/4216) | `switch (static_cast<enum>(packed_field))` + second field read in case body crashes SMT encoding (`mk_eq` bitvector width mismatch in `bitwuzla_conv.cpp:512` / `z3_conv.cpp:756`) | closed by [#4217](https://github.com/esbmc/esbmc/pull/4217) — but two crashes persist; see #4232 | — |
| [#4232](https://github.com/esbmc/esbmc/issues/4232) | `mk_eq` / `to_solver_smt_ast` crash persists after #4217: bitfield-base struct + switch-case + member read (two variants: Crash A → `to_solver_smt_ast, smt_ast.h:111`; Crash B → `mk_eq, bitwuzla_conv.cpp:512`) | **fixed** by [#4233](https://github.com/esbmc/esbmc/pull/4233) (merged) — aggregate-init flatten for bitfield-base derived structs | (workaround removed for simple aggregate-init case; see #4234 for the `std::bit_cast` variant) |
| [#4234](https://github.com/esbmc/esbmc/issues/4234) | `switch(static_cast<enum>(bit_cast member))` + `at()` in case body crashes `mk_eq` (`bitwuzla_conv.cpp:512`) — trigger is fall-through switch-case label not normalised in `adjust_switch_case_ops` | **fixed** by [#4235](https://github.com/esbmc/esbmc/pull/4235) — recurse into fall-through chain body before returning | (workaround removed; production switch now encodes correctly) |
| [#4237](https://github.com/esbmc/esbmc/issues/4237) | Value-initialising `struct Derived : class Base` via `{}` crashes `to_solver_smt_ast` (smt_ast.h:111); `Derived d;` (default-init) works correctly | **fixed** by [#4238](https://github.com/esbmc/esbmc/pull/4238) | (workaround removed; `ctrl{}` now constructs cleanly) |
| [#4240](https://github.com/esbmc/esbmc/issues/4240) | `--overflow-check` / `--ub-shift-check` generating false-positive signed-shl VCCs under `--std c++20` (C++20 [expr.shift]/2 defines signed left-shift wrapping for all inputs; ESBMC was still applying pre-C++20 rules) | **fixed** by [#4241](https://github.com/esbmc/esbmc/pull/4241) | (no workaround needed; repro: `esbmc_bug_repros/signed_shift_result_overflow.cpp`) |
| [#4243](https://github.com/esbmc/esbmc/issues/4243) | bundled `<array>` value-init (`{}`) does not zero-initialise `elems` — two violations: (1) bundled `class array` with `private: elems[N]` is not an aggregate, violating [array.overview]; (2) ESBMC skips the zero-init step of value-initialisation for non-user-provided default constructors ([dcl.init.general]/8), leaving elements as nondet and causing false-positive overflow VCCs on SMA filter accumulators | **fixed** by [#4244](https://github.com/esbmc/esbmc/pull/4244) — `elems` made `public`, restoring aggregate status (merged 2026-05-02) | `<array>` shim retained (pending ESBMC version bump) |
| [#4245](https://github.com/esbmc/esbmc/issues/4245) | `[C++ OM] Missing bundled headers: <optional> and <chrono>` — ESBMC's bundled C++ library (`src/cpp/library/`) does not provide `<optional>` (C++17) or `<chrono>` (C++11); `<ratio>` and `<variant>` are also absent. Blocks direct compilation of `nsm_type_3.cpp` and `nsm_type_5.cpp` from production source. | **open** (filed 2026-05-02) | thin `stubs/optional` and `stubs/chrono` shims added; harnesses for F-6/F-7/F-8/F-10 use inline-model approach for the logic blocked by these headers |
| [#2789](https://github.com/esbmc/esbmc/issues/2789) | negative shift distance (`x << y`, `y < 0`) not flagged under `--overflow-check`; only caught by `--ub-shift-check` | **fixed** by [#4242](https://github.com/esbmc/esbmc/pull/4242) (merged 2026-05-02) — extends negative-shift-distance UB check to fire under `--overflow-check` | — |

Every workaround site is tagged `// WORKAROUND esbmc#<n>` pointing at the
specific open issue listed in the table above. Removing a workaround is a
mechanical `grep` once the corresponding upstream fix lands; the tags are
kept narrow so multiple fixes can be reaped independently.

## What was deferred and why

- **Full Control dispatch path** — fully upgraded, no workarounds. `VerifControl ctrl{}`
  value-initialises cleanly (esbmc/esbmc#4237 fixed by #4238). The `mctp_dispatch`
  harness compiles `pdk-mctp-platforms-control.cpp` as-is and calls the
  production `on_set_endpoint_id()` directly via a `VerifControl` thin subclass
  (esbmc/esbmc#4214 fixed by #4215; #4232 by #4233; #4234 by #4235; #4237 by #4238).
  The proof covers the full dispatch path end-to-end (275 VCC, VERIFICATION
  FAILED at `cur_eid.at(2)`). Calling `ctrl.process()` directly is still
  blocked by a `dereference.cpp:1358` assertion on the variable-index
  `_routing_map.at(entry_in_map)` loop in `on_get_routing_table_entry` (dead
  code on a SetEpId packet but still inlined by ESBMC).
- **FreeRTOS-backed code** (`src/nv/ipc/queue.cpp`, `src/nv/ipc/event.cpp`,
  `src/nv/ipc/timer.cpp`) — the actual logic is in
  `src/sys/x86/sys/ipc/queue.cpp`, which delegates to `xQueueSendToBack`,
  `xQueueReceive`, etc. Modeling FreeRTOS queues is a project of its own;
  out of scope for the initial sweep.
- **Ada units** (`*.ads`, `*.adb`) — ESBMC has no Ada frontend. These will
  need to be stubbed at the C ABI boundary if they're ever in scope.

## Suggested next steps

1. **Fix F-1**: add the `interface >= UsEnd` guard to `set_cur_eid()` (or
   tighten `Validator::validate()` to reject `>= UsEnd`). F-1 is confirmed
   reachable — a Control SetEpId Request with `priv.packet_interface ∈ [2, 17]`
   will abort the firmware under `-fno-exceptions`.
2. **Fix F-5**: add the `byte_index >= bitmask.size()` guard to `set_bit` and
   `unset_bit` on `std::array<uint8_t, NvMctpEventSupportedNum>` — matching the
   guard `get_bit` already carries. Low urgency (no current OOB call site), but
   straightforward one-line fix.
3. ~~**Simplify `mctp_dispatch` once esbmc#4237 is fixed**~~ — **done** (`ctrl{}` workaround removed after esbmc/esbmc#4238 merged).
4. ~~Expand coverage to `nsm_type_3.cpp`~~ — **done** (`nsm_type3` / `nsm_type3_func`, 37 VCC Phase 1, k=9 Phase 2).
5. ~~Expand coverage to `ntc_table.{h,cpp}`~~ — **done** (`ntc_table` / `ntc_table_func`, 227 VCC Phase 1, k=9 Phase 2, all five conversion functions verified).
6. ~~Expand coverage to `soc_pwr_smoothing/presets.{h,cpp}`~~ — **done** (`pwr_smooth_params` / `pwr_smooth_params_func`, 72 VCC Phase 1, k=1 Phase 2: round-trip identity and param-id characterisation).
7. ~~Expand coverage to `fru.cpp`~~ — **done** (`fru_utils` / `fru_utils_func`, 76 VCC Phase 1, k=9 Phase 2: checksum contract and 6-bit ASCII decode output-range invariant).
8. ~~Expand coverage to `soc_pwr_smoothing/soc_sma_filter_ch.h`~~ — **done** (`soc_sma_filter` / `soc_sma_filter_func`, 504 VCC Phase 1, k=1 Phase 2: steady-state identity and output-bounded contracts).
9. ~~Expand coverage to `soc_pwr_smoothing/debug_telemetry_sma_ch.h`~~ — **done** (`debug_telemetry_sma` / `debug_telemetry_sma_func`, 261 VCC Phase 1, k=1 Phase 2: index-bounded and output-non-negative contracts).
10. ~~Expand coverage to `nv/emulation/pca9555.{h,cpp}`~~ — **done** (`pca9555` / `pca9555_func`, 1292 VCC Phase 1, k=2 Phase 2: direction constraint, masked input update, interrupt default, output propagation).
11. ~~Expand coverage to `nv/i2c/emc1812.{h,cpp}`~~ — **done** (`emc1812` / `emc1812_func`, 52 VCC Phase 1, k=1 Phase 2: `int8_t↔uint8_t` threshold cast round-trip identity for all four set/get pairs).
12. ~~Expand coverage to `nv/i2c/tmp1075.{h,cpp}`~~ — **done** (`tmp1075` / `tmp1075_func`, 33 VCC Phase 1, k=1 Phase 2: 12-bit temperature encoding round-trip `int8_t → <<4 → uint16_t → >>4 → int8_t` and temp_read_cast).
13. ~~Expand coverage to `nv/i2c/tmp461.{h,cpp}`~~ — **done** (`tmp461` / `tmp461_func`, 57 VCC Phase 1, k=1 Phase 2: `int8_t↔uint8_t` cast round-trip for all four threshold set/get pairs).
14. ~~Drop `--no-unwinding-assertions` and set per-target loop bounds~~ —
    **done**. Removed the flag from all Phase 1 targets; per-target `--unwind N`
    set to the exact array/table size (`nsm_type_2` → 12, `nsm_type3` → 9,
    `telemetry` → 11, `spi_utils` → 9, `i2c_crc8` → 257, `fru_utils` → 9;
    `ntc_table` already at 9). All 22 targets in `make all` pass with
    unwinding assertions active — no new bugs found.
15. Stand up a CI hook that runs `make all` on every PR; verification must
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
make nsm_type5_f8_neg       # F-8: gpio ei_entries[16] OOB (expect VERIFICATION FAILED)
make nsm_type3_f10_neg      # F-10: silent 125°C substitution — real ntc_table.cpp (expect VERIFICATION FAILED)
make debug_telemetry_f13_neg  # F-13: negative percent wrap to uint8 (expect VERIFICATION FAILED)
make pca9555_f14_neg        # F-14: retracted — production pca9555.cpp (expect VERIFICATION SUCCESSFUL)
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
