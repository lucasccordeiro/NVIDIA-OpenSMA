# OpenSMA ESBMC Verification — Initial Report

**Date**: 2026-04-25
**Tool**: ESBMC 8.2.0 (aarch64-macos)
**Scope**: bounded model checking of selected modules in
[NVIDIA/OpenSMA](https://github.com/NVIDIA/OpenSMA)

## TL;DR

Three modules verified end-to-end against language-level safety properties
(pointer/bounds/overflow/div-by-zero/memory-leak) and against module-specific
functional contracts via k-induction. One real codebase finding:
`pdk::mctp::platforms::set_cur_eid()` will `std::terminate()` on an
out-of-range `interface` argument. Three ESBMC C++-frontend bugs filed
against [esbmc/esbmc](https://github.com/esbmc/esbmc); workarounds in place
so verification continues.

## What was verified

| Target | Source | Phase 1 | Phase 2 | Negative test |
|---|---|:-:|:-:|:-:|
| MCTP packet parser | `corepdk/.../app/pdk-mctp-app-packet.cpp` | ✅ 62 VCC | ✅ 76 VCC, k=1 | ✅ CEX on undersized input |
| MCTP routing helpers | `corepdk/.../platforms/x86/pdk-mctp-platforms-router-plat.cpp` | ✅ 70 VCC | ✅ 66 VCC, k=1 | ✅ CEX on `iface == UsEnd` |
| Fixed-point arithmetic | `src/nv/common/fixed_point.h` | ✅ 78 VCC | ✅ 24 VCC, k=1 | — |

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

### F-1 — `set_cur_eid()` lacks bounds check (medium)

**File**: `corepdk/modules/mctp-cpp/src/platforms/x86/pdk-mctp-platforms-router-plat.cpp:34`

```cpp
void set_cur_eid(RoutingTable& routing_table,
                 Packet::InterfaceType interface, uint8_t eid)
{
    routing_table.ec.cur_eid.at(interface) = eid;
}
```

Compare with the sibling `get_cur_eid` (line 24), which guards with
`if (interface >= static_cast<uint16_t>(Interface::UsEnd))`. `set_cur_eid`
has no such guard and relies on `std::array::at()` to throw
`std::out_of_range` if `interface` is out of bounds. The throw is uncaught
on every reachable call path in this codebase → `std::terminate()` and
firmware reset.

ESBMC's `mctp_router_neg` harness produces a deterministic counterexample:
calling `set_cur_eid(table, UsEnd, ...)` violates the array bound at
`pdk-mctp-platforms-router-plat.cpp:36` (verified against our shim's
`std::array::at`).

**Suggested fix**: mirror `get_cur_eid`'s guard:

```cpp
if (interface >= static_cast<uint16_t>(Interface::UsEnd)) {
    return;
}
routing_table.ec.cur_eid.at(interface) = eid;
```

### Items checked, no defects

- Packed-struct alignment access in `Packet::to_span()` and `Packet::from()`
  (ESBMC reports the standard 6 packed-struct alignment warnings; these are
  expected for a `[[gnu::packed]]` MCTP wire format and not bugs).
- All `nv::fixed_point` conversion functions are total over their declared
  input ranges; no overflow under the documented preconditions.

## Tooling-level findings (ESBMC bugs)

Three C++ frontend bugs were discovered during harness development. Each
has a freestanding minimal reproducer under `verification/esbmc_bug_repros/`
and is upstream:

1. **[esbmc/esbmc#4180](https://github.com/esbmc/esbmc/issues/4180)** —
   two converter assertion failures:
   - `std::array<T, N>` instantiation crashes
     `gen_vptr_initializations` (3-line repro).
   - Namespace-qualified `constexpr` initialiser crashes
     `getAsType` (`NestedNameSpecifierBase.h:159`) (24-line repro).
2. **[esbmc/esbmc#4182](https://github.com/esbmc/esbmc/issues/4182)** —
   `using ns::T;` for a class **or** enum type triggers
   `Conversion of unsupported clang type: Using` (16-line repro plus a
   follow-up comment with the enum-type variant).

Workarounds applied in `verification/stubs/`:

- `<array>`, `<span>`, `<bit>` — minimal POD shims that expose only the
  surface OpenSMA actually uses.
- `verification/stubs/app/pdk-mctp-app-packet*.h` — overlay headers that are
  byte-identical to production except for a `using` declaration in lieu of
  the `platforms::TransmitUnit` qualifier and `reinterpret_cast` in lieu
  of `std::bit_cast` inside class methods.
- Type-aliases (`using T = ns::T;`) instead of using-declarations
  (`using ns::T;`) for class/enum types in every harness.

Every workaround site is tagged `// WORKAROUND esbmc#<n>` for easy removal
once the upstream fixes land.

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

1. Land the suggested fix for F-1 (`set_cur_eid` bounds check) — small,
   isolated, and removes a real terminate path.
2. Add a fourth target: `nv::common::expected` or `nv::common::utils` —
   header-only, broad reuse, low overlay cost.
3. Once esbmc#4180 closes: revisit `pdk-mctp-app-validator.cpp`, then walk
   down the `nsm_type_*.cpp` family in `src/nv/mctp/` (similar parser
   shape; the harness template will transfer).
4. Stand up a CI hook that runs `make all` on every PR; verification must
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
