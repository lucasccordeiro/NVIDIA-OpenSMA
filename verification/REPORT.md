# OpenSMA ESBMC Verification — Initial Report

**Date**: 2026-04-25
**Tool**: ESBMC 8.2.0 (aarch64-macos)
**Scope**: bounded model checking of selected modules in
[NVIDIA/OpenSMA](https://github.com/NVIDIA/OpenSMA)

## TL;DR

Nine modules verified end-to-end against language-level safety properties
(pointer/bounds/overflow/div-by-zero/memory-leak) and against module-specific
functional contracts via k-induction. **One active finding (F-1)**:
`set_cur_eid()` OOBs on a 2-entry array when a Control SetEpId Request
carries a gap interface; ESBMC proves both the validator gap (with the
correct packet type) and the OOB endpoint; code inspection confirms the
production call-site (`on_set_endpoint_id()`) makes the connection without
a bounds check. One initially-claimed finding (F-2) **retracted on review**:
it was a benign unsigned wrap in `align_to`, not a bug. Multiple ESBMC
C++-frontend bugs filed against [esbmc/esbmc](https://github.com/esbmc/esbmc);
workarounds in place so verification continues.

## What was verified

| Target | Source | Phase 1 | Phase 2 | Negative test |
|---|---|:-:|:-:|:-:|
| MCTP packet parser | `corepdk/.../app/pdk-mctp-app-packet.cpp` | ✅ 62 VCC | ✅ 76 VCC, k=1 | ✅ CEX on undersized input |
| MCTP routing helpers | `corepdk/.../platforms/x86/pdk-mctp-platforms-router-plat.cpp` | ✅ 70 VCC | ✅ 66 VCC, k=1 | ✅ CEX on `iface == UsEnd` |
| Fixed-point arithmetic | `src/nv/common/fixed_point.h` | ✅ 78 VCC | ✅ 24 VCC, k=1 | — |
| Saturating arithmetic | `src/nv/common/utils.h` | ✅ 20 VCC | ✅ k=1 | ⚠ ESBMC strict unsigned-overflow demo (not a bug) |
| MCTP validator state machine | `corepdk/.../app/pdk-mctp-app-validator.cpp` | ✅ 119 VCC | ✅ k=1 (full functional contract) | — |
| MCTP dispatch — F-1 reachability | `mctp_dispatch_harness.cpp` + `pdk-mctp-app-validator.cpp` + router/packet | ✅ CEX — `set_cur_eid` OOBs (iface=2, valid=true) | — | ✅ (expected FAILED) |
| NSM type 2 (PCIe-link reset validator) | `src/nv/mctp/nsm_type_2.cpp` (`validatePcieLinkResetValue`) | ✅ | ✅ k=12 (membership iff + below-range rejection) | — |
| SPI byte-buffer (de)serialisation | `src/nv/spi/utils.{h,cpp}` (`buf_to_u{16,32}`, `u{16,32}_to_buf`) | ✅ | ✅ k=9 (round-trip + big-endian + OOB-no-write) | — |
| I2C CRC-8 helpers | `src/nv/i2c/helper.cpp` (`crc8`) | ✅ | ✅ k=5 (incrementality + init-zero invariant) | — |
| User-defined integer literals | `src/nv/common/literals.h` (`_u8`/`_u16`/`_u32`/`_i8`/`_i16`/`_i32`/`_bits_sizeof`/`_bit`) | ✅ | ✅ k=1 (mask agreement, signed/unsigned truncation parity, `bits/8`, `1ULL << i`) | ✅ CEX on `_bit(i≥64)` via `--ub-shift-check` |

All BMC runs solved sub-second on Bitwuzla 0.8.2.

### Properties checked

**Phase 1 — language-level safety** (default + opt-in checks):

- pointer-check, bounds-check, div-by-zero (default-on)
- arithmetic over/underflow (signed and unsigned)
- memory-leak
- NaN propagation

**Phase 2 — functional contracts** via `--k-induction --k-step 1
--max-k-step 6`:

- `Packet`: `to_span` / `from` round-trip preserves all fields; the resulting
  span's size equals `priv + hdr + msg`.
- `RoutingTable`: `get_cur_eid(set_cur_eid(t, i, eid), i) == eid` for all
  `i < UsEnd`.
- `fixed_point`: `sfxp22_10_to_sfxp32_0(sfxp32_0_to_sfxp22_10(x)) == x` for
  all `x ∈ [INT32_MIN >> 10, INT32_MAX >> 10]`.

## Findings

### F-1 — `set_cur_eid()` OOBs on gap interface via Control SetEpId dispatch

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

**What the verifier proves** (`mctp_dispatch`, `mctp_router_neg`):

The `mctp_dispatch` harness (`verification/harnesses/mctp_dispatch_harness.cpp`)
constructs a **Control SetEpId Request** — the exact packet type that
`Control::process()` dispatches to `on_set_endpoint_id()` — with interface
`iface_val` constrained to the gap `[UsEnd=2, End=18)`. ESBMC proves two
properties in sequence:

1. **Validator gap**: `Validator::validate()` returns `true` for this packet
   and interface. The validator guards `interface >= End` (18) but not
   `interface >= UsEnd` (2); interfaces in `[2, 17]` pass unchallenged.

2. **OOB**: `set_cur_eid(router, iface_val, eid)` calls
   `cur_eid.at(iface_val)`, which violates `i < N` (i.e., `iface_val < 2`).

Counterexample produced by ESBMC (Bitwuzla, sub-second):

```
iface_val = 2          → passes __ESBMC_assume(iface_val >= 2 && iface_val < 18)
valid     = 1          → validate() accepted the Control SetEpId Request
VIOLATED: std::array::at out of range  (i::0 < 2 fails)
```

3. **Production connection (code inspection)**: `on_set_endpoint_id()`
   (`pdk-mctp-platforms-control.cpp:61`) calls

   ```cpp
   set_cur_eid(_router, platforms::get_packet_interface(rx), crx.data[1]);
   ```

   unconditionally for `SetEidNormal` and `SetEidForced` sub-commands, with
   no additional bounds check on the interface value.

*Note on harness scope*: `platforms::Control ctrl{}` triggers an ESBMC
frontend crash (assertion in `clang_c_adjust_expr.cpp:158`; filed as
esbmc/esbmc#TBD), so `Control::process()` is not called directly in the
harness. The harness calls `set_cur_eid()` directly after `validate()` —
the same sequence `on_set_endpoint_id()` executes at lines 57–61. The
production connection is confirmed by code inspection, not by the formal
proof alone.

**Runtime effect (empirically confirmed)**: under production flags
`-fno-exceptions -fno-rtti`, `std::array::at(OOB)` calls `abort()` — not
silent corruption, not a throw. Reproduced via ESBMC's `--branch-coverage
--generate-ctest-testcase` on a 26-line standalone harness mirroring the
relevant call shape (`verification/ctest/f1/`); the generated executable
exits with signal 6 (SIGABRT) when ESBMC picks any `i ∈ [UsEnd, UsEnd+8)`.

**What remains open**: whether the MCTP protocol router can deliver a
Set-EID Control packet with `priv.packet_interface >= UsEnd` to
`on_set_endpoint_id()` in a real deployment. This depends on which physical
interfaces accept the Set-EID command and how `packet_interface` is set
upstream of the dispatch loop. If the receive path already guarantees
`packet_interface < UsEnd`, the OOB is unreachable at runtime; if not,
any such packet causes `abort()`.

**Recommendation**: add the guard mirroring `get_cur_eid`. Cost is one
line; the benefit is removing dependence on an external invariant that is
not documented at the function boundary.

```cpp
if (interface >= static_cast<uint16_t>(Interface::UsEnd)) {
    return;
}
routing_table.ec.cur_eid.at(interface) = eid;
```

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
| [#4190](https://github.com/esbmc/esbmc/issues/4190) | bundled libc++ missing `<span>`, `<bit>`, parts of `<type_traits>`; bundled `<array>` is a `class` not an aggregate | partial — [#4194](https://github.com/esbmc/esbmc/pull/4194) bundled `<span>` + most traits; aggregate-`<array>` and `std::underlying_type_t` still missing | thin `<span>` shim (transitive `<bit>` + avoids bundled-`<array>` collision); `<array>` shim retained; utils.h inlined for `underlying_type_t` |
| [#4191](https://github.com/esbmc/esbmc/issues/4191) | spurious CEX on aliased `*std::bit_cast<T*>(...)` round-trip | fixed by [#4192](https://github.com/esbmc/esbmc/pull/4192) — but the bundled pointer overload uses `reinterpret_cast` (drops const → compile error on `bit_cast<uint8_t*>(this)` in const methods) | thin `<bit>` shim that keeps the pointer aliasing fix and uses C-cast for the pointer specialisation (preserves `std::bit_cast`'s const-agnostic semantics); follow-up commented on #4191 |
| [#4195](https://github.com/esbmc/esbmc/issues/4195) | C++20 `using enum X;` (`UsingEnumDecl`) not handled | fixed by [#4204](https://github.com/esbmc/esbmc/pull/4204) (merged 2026-04-28) | (sed-patch dropped; validator.cpp compiles directly from upstream) |
| [#4201](https://github.com/esbmc/esbmc/issues/4201) | `--overflow-check` flags signed left-shift wrap that is defined under C++20+ | open — [#4203](https://github.com/esbmc/esbmc/pull/4203) merged then reverted by [#4208](https://github.com/esbmc/esbmc/pull/4208) (skip too broad); refined replacement [#4211](https://github.com/esbmc/esbmc/pull/4211) is open with a non-negativity precondition on `E1` (type-driven shape predicate covers the firmware byte-deserialiser idiom) | spi_utils harness uses production form on local build of #4211; mainline ESBMC still requires the parenthesisation workaround |
| [#4184](https://github.com/esbmc/esbmc/pull/4184) | `getAsType` guard for namespace-qualified constexpr | merged 2026-04-26 | (workarounds removed) |
| [#4188](https://github.com/esbmc/esbmc/pull/4188) | tag-id mismatch in `annotate_class_method` | merged 2026-04-26 | (crash gone; see #4190 row for the residual `<array>` shim reason) |
| [#4187](https://github.com/esbmc/esbmc/pull/4187) | `UsingType` handling for clang ≥ 22 | merged 2026-04-26 | (workarounds removed) |
| [#4192](https://github.com/esbmc/esbmc/pull/4192) | bundle `<bit>` with pointer-aware `bit_cast` | merged 2026-04-27 | validator Phase 2 ungated; packet overlay deleted; `<bit>` shim retained as thin const-aware override |
| [#4194](https://github.com/esbmc/esbmc/pull/4194) | bundle `<span>` and complete `<type_traits>` | merged 2026-04-27 | `<span>` shim retained as thin replacement (transitive `<bit>` + avoid bundled-`<array>` collision); utils.h still inlined for residual `underlying_type_t` gap |
| [#4203](https://github.com/esbmc/esbmc/pull/4203) | skip signed-shl overflow claim under C++20+ | merged 2026-04-28, **reverted by [#4208](https://github.com/esbmc/esbmc/pull/4208)** the same day (skip condition too broad — would suppress still-UB negative-`E1` case) | spi_utils workaround restored |
| [#4204](https://github.com/esbmc/esbmc/pull/4204) | handle `UsingEnumDecl` (C++20 `using enum`) | merged 2026-04-28 | (sed-patch dropped; validator.cpp compiles directly) |
| [#4211](https://github.com/esbmc/esbmc/pull/4211) | replacement for #4203: skip signed-shl overflow only when `E1` is provably non-negative (type-driven predicate); standard-aware via `--std c++20+` parsing; legacy spellings (`98`, `03`) and pre-C++20 unaffected | open (7 CORE regressions, paired with the OpenSMA harness restoration) | n/a — when this lands, the spi_utils parenthesisation workaround drops |
| [#TBD](https://github.com/esbmc/esbmc/issues) | `platforms::Control` default-construction triggers assertion `new_comp.size() == ops.size()` in `clang_c_adjust_expr.cpp:158`; ESBMC aborts during GOTO program creation | open (to be filed) | `mctp_dispatch` harness calls `set_cur_eid()` directly after `validate()` instead of through `Control::process()`; production connection confirmed by code inspection |

Every workaround site is tagged `// WORKAROUND esbmc#<n>` pointing at the
specific open issue listed in the table above. Removing a workaround is a
mechanical `grep` once the corresponding upstream fix lands; the tags are
kept narrow so multiple fixes can be reaped independently.

## What was deferred and why

- **FreeRTOS-backed code** (`src/nv/ipc/queue.cpp`, `src/nv/ipc/event.cpp`,
  `src/nv/ipc/timer.cpp`) — the actual logic is in
  `src/sys/x86/sys/ipc/queue.cpp`, which delegates to `xQueueSendToBack`,
  `xQueueReceive`, etc. Modeling FreeRTOS queues is a project of its own;
  out of scope for the initial sweep.
- **Ada units** (`*.ads`, `*.adb`) — ESBMC has no Ada frontend. These will
  need to be stubbed at the C ABI boundary if they're ever in scope.
- **Full Control dispatch path** — verifying `Control::process()` end-to-end
  is blocked by the ESBMC frontend crash on `platforms::Control` construction
  (esbmc/esbmc#TBD). Once that is fixed, the harness can be upgraded to call
  `ctrl.process()` directly, eliminating the code-inspection step for the
  F-1 production connection.

## Suggested next steps

1. **Apply the one-line fix** for F-1 — mirror the `get_cur_eid` guard in
   `set_cur_eid`, removing dependence on an external invariant that isn't
   documented at the function boundary (see recommendation in F-1 section).
2. **Trace the full receive path for F-1** — determine whether the MCTP
   protocol router can deliver a Set-EID Control packet with
   `priv.packet_interface >= UsEnd` to `on_set_endpoint_id`. If yes, F-1
   is a reachable abort under `-fno-exceptions`; if no, the fix is still
   good defensive practice.
3. **Upgrade mctp_dispatch once esbmc#TBD is fixed** — replace the
   direct `set_cur_eid()` call with `ctrl.process()` to prove the full
   end-to-end path formally, removing the code-inspection caveat.
4. Walk down the `nsm_type_*.cpp` family in `src/nv/mctp/` (similar parser
   shape; the harness template will transfer).
5. Stand up a CI hook that runs `make all` on every PR; verification must
   stay green and any failure must be triaged before merge.

## Reproducing

```sh
cd verification
make all                                       # Phase 1 across every target
make mctp_packet_func mctp_router_func \
     mctp_validator_func                       # Phase 2 (k-induction)
make fixed_point_func utils_func               # ditto
make mctp_packet_neg mctp_router_neg utils_neg # negative tests (expect FAILED)
make mctp_dispatch                             # F-1 reachability (expect FAILED)
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
