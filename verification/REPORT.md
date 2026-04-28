# OpenSMA ESBMC Verification — Initial Report

**Date**: 2026-04-25 (updated 2026-04-29)
**Tool**: ESBMC 8.2.0 (aarch64-macos)
**Scope**: bounded model checking of selected modules in
[NVIDIA/OpenSMA](https://github.com/NVIDIA/OpenSMA)

## TL;DR

Nine modules verified end-to-end against language-level safety properties
(pointer/bounds/overflow/div-by-zero/memory-leak) and against module-specific
functional contracts via k-induction. **One vulnerability formally proven
reachable** (F-1) via `mctp_dispatch` — ESBMC finds a counterexample where a
well-formed PLDM packet with a gap interface triggers `set_cur_eid()` to
OOB-index the 2-entry `cur_eid` array. Two initially-claimed findings (F-2,
F-3) **retracted on review**. Several ESBMC C++-frontend bugs filed against
[esbmc/esbmc](https://github.com/esbmc/esbmc); most are now fixed and merged;
workarounds removed where applicable.

## What was verified

| Target | Source | Phase 1 | Phase 2 | Negative test |
|---|---|:-:|:-:|:-:|
| MCTP packet parser | `corepdk/.../app/pdk-mctp-app-packet.cpp` | ✅ 62 VCC | ✅ 76 VCC, k=1 | ✅ CEX on undersized input |
| MCTP routing helpers | `corepdk/.../platforms/x86/pdk-mctp-platforms-router-plat.cpp` | ✅ 70 VCC | ✅ 66 VCC, k=1 | ✅ CEX on `iface == UsEnd` |
| MCTP dispatch (F-1 reachability) | `Validator::validate()` + `set_cur_eid()` | — | — | ✅ CEX: `iface_val=2`, `valid=true`, OOB at `cur_eid.at(2)` |
| Fixed-point arithmetic | `src/nv/common/fixed_point.h` | ✅ 78 VCC | ✅ 24 VCC, k=1 | — |
| Saturating arithmetic | `src/nv/common/utils.h` | ✅ 20 VCC | ✅ k=1 | ⚠ ESBMC strict unsigned-overflow demo (not a bug) |
| MCTP validator state machine | `corepdk/.../app/pdk-mctp-app-validator.cpp` | ✅ 119 VCC | ✅ k=1 (full functional contract) | — |
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

**What ESBMC proved** (`mctp_dispatch` harness, `LANG_FLAGS`, 179 VCC / 10
remaining after simplification; Bitwuzla solves in <0.01 s):

The harness constrains `iface_val` to `[UsEnd=2, End=18)` and builds a
minimal PLDM start-of-message that satisfies every guard inside `validate()`
(`hdr_ver==0x01`, `dst_eid==NULL_EID`, `msg_type==Pldm`, `som==1`). ESBMC
finds the counterexample immediately:

```
State 5  iface_val = 2   (Interface::DsI2c0, first gap value)
State 8  valid = 1       (Validator::validate() returns true)
State 9  Violated: std::array::at out of range  [i::0 < 2  fails]
         at set_cur_eid() → cur_eid.at(2) on a size-2 array
```

Any `iface_val ∈ [2, 17]` triggers the same path.

**Runtime effect (empirically confirmed)**: under production flags
`-fno-exceptions -fno-rtti`, `std::array::at(OOB)` calls `abort()` — not
silent corruption, not a throw. Reproduced via ESBMC's `--branch-coverage
--generate-ctest-testcase` on a 26-line standalone harness (`ctest/f1/`);
the generated executable exits with signal 6 (SIGABRT).

**Recommendation**: this is a confirmed reachable abort path for any
well-formed PLDM (or Control Request) packet whose `priv.packet_interface`
falls in `[UsEnd, End)`. Add the guard that `get_cur_eid` already carries:

```cpp
if (interface >= static_cast<uint16_t>(Interface::UsEnd)) {
    return;
}
routing_table.ec.cur_eid.at(interface) = eid;
```

Alternatively, tighten `Validator::validate()` to reject `interface >=
Interface::UsEnd` instead of `>= Interface::End`.

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

## Suggested next steps

1. **Fix F-1**: add the `interface >= UsEnd` guard to `set_cur_eid()` (or
   tighten `Validator::validate()` to reject `>= UsEnd`). F-1 is confirmed
   reachable — any PLDM packet with `priv.packet_interface ∈ [2, 17]` will
   abort the firmware under `-fno-exceptions`.
2. Expand coverage to the `nsm_type_*.cpp` family in `src/nv/mctp/` (similar
   parser shape; the harness template transfers directly).
3. Stand up a CI hook that runs `make all` on every PR; verification must
   stay green and any failure must be triaged before merge.

## Reproducing

```sh
cd verification
make all                 # all Phase 1 targets
make mctp_packet_func    # Phase 2 (k-induction)
make mctp_router_func
make fixed_point_func
make mctp_packet_neg     # negative tests (expect VERIFICATION FAILED)
make mctp_router_neg
make mctp_dispatch       # F-1 reachability proof (expect VERIFICATION FAILED)
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
