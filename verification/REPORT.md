# OpenSMA ESBMC Verification — Initial Report

**Date**: 2026-04-25
**Tool**: ESBMC 8.2.0 (aarch64-macos)
**Scope**: bounded model checking of selected modules in
[NVIDIA/OpenSMA](https://github.com/NVIDIA/OpenSMA)

## TL;DR

Four modules verified end-to-end against language-level safety properties
(pointer/bounds/overflow/div-by-zero/memory-leak) and against module-specific
functional contracts via k-induction. **One defensive-programming
observation** (F-1, reachability not traced — see below). One initially-claimed
finding (F-2) **retracted on review**: it was a benign unsigned wrap in
`align_to`, not a bug. Three ESBMC C++-frontend bugs filed against
[esbmc/esbmc](https://github.com/esbmc/esbmc); workarounds in place so
verification continues.

## What was verified

| Target | Source | Phase 1 | Phase 2 | Negative test |
|---|---|:-:|:-:|:-:|
| MCTP packet parser | `corepdk/.../app/pdk-mctp-app-packet.cpp` | ✅ 62 VCC | ✅ 76 VCC, k=1 | ✅ CEX on undersized input |
| MCTP routing helpers | `corepdk/.../platforms/x86/pdk-mctp-platforms-router-plat.cpp` | ✅ 70 VCC | ✅ 66 VCC, k=1 | ✅ CEX on `iface == UsEnd` |
| Fixed-point arithmetic | `src/nv/common/fixed_point.h` | ✅ 78 VCC | ✅ 24 VCC, k=1 | — |
| Saturating arithmetic | `src/nv/common/utils.h` | ✅ 20 VCC | ✅ k=1 | ⚠ ESBMC strict unsigned-overflow demo (not a bug) |
| MCTP validator state machine | `corepdk/.../app/pdk-mctp-app-validator.cpp` | ✅ 119 VCC | ✅ k=9 (reset contract; bit_cast-dependent contracts gated until [#4192](https://github.com/esbmc/esbmc/pull/4192) lands) | — |

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

### F-1 — `set_cur_eid()` lacks bounds check (defensive observation; reachability not traced)

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

**What the verifier shows**: `mctp_router_neg` produces a deterministic
counterexample where `interface == UsEnd` violates an `__ESBMC_assert(i < N,
...)` baked into the verification-only `<array>` shim's `at()`. This is a
*model-checking artefact* derived from the shim, not a direct proof of a
production failure mode.

**Runtime effect (empirically confirmed)**: under production flags
`-fno-exceptions -fno-rtti`, `std::array::at(OOB)` calls `abort()` — not
silent corruption, not a throw. Reproduced via ESBMC's `--branch-coverage
--generate-ctest-testcase` on a 26-line standalone harness mirroring the
relevant call shape (`verification/ctest/f1/`); the generated executable
exits with signal 6 (SIGABRT) when ESBMC picks any `i ∈ [UsEnd, UsEnd+8)`.

So *if* a caller delivers `interface >= UsEnd` to `set_cur_eid`, the firmware
resets. Whether that path is reachable is the open question (below).

**What I did NOT verify**:
- **Reachability**. The only non-test caller is
  `Control::on_set_endpoint_id` (`pdk-mctp-platforms-control.cpp:61`),
  which forwards `platforms::get_packet_interface(rx)` — a wire-supplied
  `uint8_t` from `priv.packet_interface`. Whether the MCTP dispatch path
  can deliver `interface >= UsEnd` to that handler depends on the protocol
  router (which interfaces accept Set-EID; how `packet_interface` is
  validated upstream). I have not traced this. If the dispatch path
  already restricts `interface < UsEnd` before this handler runs, F-1
  reduces to an internally-impossible state and is purely a
  defensive-programming nit.

**Recommendation**: add the guard mirroring `get_cur_eid`. Cost is one
line; the benefit is removing dependence on an external invariant that
isn't documented at the function boundary. Whether to file as a CVE-class
finding requires the reachability trace I haven't done.

```cpp
if (interface >= static_cast<uint16_t>(Interface::UsEnd)) {
    return;
}
routing_table.ec.cur_eid.at(interface) = eid;
```

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
| [#4190](https://github.com/esbmc/esbmc/issues/4190) | bundled libc++ missing `<span>`, `<bit>`, parts of `<type_traits>`; bundled `<array>` is a `class` not an aggregate | open | `verification/stubs/{array,span,bit}` shims; `<type_traits>` features inlined into utils harness |
| [#4191](https://github.com/esbmc/esbmc/issues/4191) | spurious CEX on aliased `*std::bit_cast<T*>(...)` round-trip | open ([fix in flight: #4192](https://github.com/esbmc/esbmc/pull/4192)) | C-cast in lieu of `std::bit_cast` in `pdk-mctp-app-packet.h` overlay; validator's bit_cast-dependent Phase 2 contracts gated behind `ESBMC_FUNCTIONAL_BIT_CAST` |
| [#4195](https://github.com/esbmc/esbmc/issues/4195) | C++20 `using enum X;` (`UsingEnumDecl`) not handled | open | build-time `sed` rewrites `validator.cpp`'s `using enum` line to per-enumerator `using` declarations; source of truth is upstream |
| [#4184](https://github.com/esbmc/esbmc/pull/4184) | `getAsType` guard for namespace-qualified constexpr | merged 2026-04-26 | (workarounds removed) |
| [#4188](https://github.com/esbmc/esbmc/pull/4188) | tag-id mismatch in `annotate_class_method` | merged 2026-04-26 | (crash gone; see #4190 row for the residual `<array>` shim reason) |
| [#4187](https://github.com/esbmc/esbmc/pull/4187) | `UsingType` handling for clang ≥ 22 | merged 2026-04-26 | (workarounds removed) |
| [#4192](https://github.com/esbmc/esbmc/pull/4192) | bundle `<bit>` with pointer-aware `bit_cast` | in flight (open PR) | (validator Phase 2 ungated, packet overlay drops, when merged) |
| [#4194](https://github.com/esbmc/esbmc/pull/4194) | bundle `<span>` and complete `<type_traits>` | in flight (open PR) | (`<span>` shim and utils.h inlining drop when merged) |

Every workaround site is tagged `// WORKAROUND esbmc#<n>` pointing at the
specific open issue listed in the table above. Removing a workaround is a
mechanical `grep` once the corresponding upstream fix lands; the tags are
kept narrow so multiple fixes can be reaped independently.

## What was deferred and why

- **`pdk-mctp-app-validator.cpp`** — depends transitively on
  `pdk-mctp-app-control.h`, `pdk-mctp-app-vendor.h`, `pdk-mctp-platforms-nsm.h`,
  and `pdk-mctp-platforms-nsm-packet.h`, each containing 2–4 `std::bit_cast`
  call sites. Verifying it would require ~360 lines of overlay-header
  maintenance — clerical workaround surface, not verification work. Will
  revisit once esbmc#4180 closes.
- **FreeRTOS-backed code** (`src/nv/ipc/queue.cpp`, `src/nv/ipc/event.cpp`,
  `src/nv/ipc/timer.cpp`) — the actual logic is in
  `src/sys/x86/sys/ipc/queue.cpp`, which delegates to `xQueueSendToBack`,
  `xQueueReceive`, etc. Modeling FreeRTOS queues is a project of its own;
  out of scope for the initial sweep.
- **Ada units** (`*.ads`, `*.adb`) — ESBMC has no Ada frontend. These will
  need to be stubbed at the C ABI boundary if they're ever in scope.

## Suggested next steps

1. Trace MCTP dispatch reachability for F-1: does the receive path ever
   deliver a Set-EID Control packet with `priv.packet_interface >= UsEnd`
   to `Control::on_set_endpoint_id`? If yes, F-1 escalates to a real
   abort/UB path under `-fno-exceptions`; if no, it stays a
   defensive-programming nit.
2. Once esbmc#4180 closes: revisit `pdk-mctp-app-validator.cpp`, then walk
   down the `nsm_type_*.cpp` family in `src/nv/mctp/` (similar parser
   shape; the harness template will transfer).
3. Stand up a CI hook that runs `make all` on every PR; verification must
   stay green and any failure must be triaged before merge.

## Reproducing

```sh
cd verification
make all                 # mctp_packet + mctp_router + fixed_point (Phase 1)
make mctp_packet_func    # Phase 2 (k-induction)
make mctp_router_func
make fixed_point_func
make mctp_packet_neg     # negative tests (expect VERIFICATION FAILED)
make mctp_router_neg
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
