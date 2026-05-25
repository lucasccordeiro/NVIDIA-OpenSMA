# Reproducing F-1 with ESBMC — `set_cur_eid()` lacks bounds check

This guide walks an independent reviewer end-to-end from a fresh machine to a
`VERIFICATION FAILED` verdict with a concrete counterexample that proves F-1
from the OpenSMA verification report. It assumes you are familiar with C/C++
and bounded model checking but not with this codebase.

The fix has already landed upstream
([NVIDIA/OpenSMA#1](https://github.com/NVIDIA/OpenSMA/issues/1#issuecomment-4417902007)).
To reproduce the original bug you must check out a commit *before* the upstream
patch — the `vr_sma` branch of [the lucasccordeiro
fork](https://github.com/lucasccordeiro/NVIDIA-OpenSMA) preserves the pre-fix
source plus the harness used in the report.

This document was written from a live re-run on 2026-05-25 against ESBMC
**8.3.0** (the version currently available locally). The original report ran on
ESBMC **8.2.0**. Both paths are documented below — §6.2 explains the one
overlay change needed to keep the proof reproducible on 8.3.0.

---

## 1. Environment

| Component                           | Tested combinations                          | Notes |
|-------------------------------------|----------------------------------------------|-------|
| OS                                  | macOS 14+ (aarch64); Linux x86_64 also fine  | The counter-example offsets here are from macOS aarch64. |
| ESBMC                               | **8.2.0** (report) or **8.3.0** (re-run)     | See §6.2 for the 8.3.0 caveat. |
| Default solver                      | Bitwuzla 0.8.2 (8.2.0) / 0.9.0 (8.3.0)       | Bundled with each ESBMC release; < 0.01 s on this benchmark. |
| C++ standard                        | C++20 (`--std c++20`)                        | Harness uses `std::array`, `std::bit_cast`, designated initialisers. |
| Compiler (host build of demonstrator only) | clang ≥ 14 or gcc ≥ 11 with libc++/libstdc++ supporting C++20 | Only for the optional runtime ctest under `verification/ctest/f1/`. |
| Make + git                          | standard                                     | Repo ships `verification/Makefile`. |
| Disk                                | ≈ 200 MB checkout, ≈ 1 GB ESBMC build        | |

### 1.1 Install ESBMC

```bash
git clone https://github.com/esbmc/esbmc.git
cd esbmc
git checkout v8.2.0    # or a later tag; v8.3.0 also works after §6.2
./scripts/configure.sh # follow distro-specific instructions in BUILD.md
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
./src/esbmc/esbmc --version
```

Export the path so the project's `Makefile` finds it:

```bash
export ESBMC=$HOME/esbmc/build/src/esbmc/esbmc
```

### 1.2 Check out OpenSMA at the pre-fix commit

```bash
git clone https://github.com/lucasccordeiro/NVIDIA-OpenSMA.git
cd NVIDIA-OpenSMA
git checkout vr_sma
sed -n '34,37p' corepdk/modules/mctp-cpp/src/platforms/x86/pdk-mctp-platforms-router-plat.cpp
```

You should see, **unguarded**:

```cpp
void set_cur_eid(RoutingTable& routing_table, Packet::InterfaceType interface, uint8_t eid)
{
    routing_table.ec.cur_eid.at(interface) = eid;
}
```

If you see an `if (interface >= ... ::UsEnd) return;` guard at the top, you are
on a post-fix commit — back up. No third-party dependencies need building;
ESBMC consumes OpenSMA source directly.

---

## 2. Understanding the target

### 2.1 Where the bug lives

- **Sink** — `pdk::mctp::platforms::set_cur_eid` at
  `corepdk/modules/mctp-cpp/src/platforms/x86/pdk-mctp-platforms-router-plat.cpp:34`.
  Writes `routing_table.ec.cur_eid.at(interface) = eid;`.
- **Array** — `cur_eid` is `std::array<uint8_t, UsEnd>` with `UsEnd == 2`
  (interfaces `UsI2c = 0`, `UsUsb = 1`).
- **Sibling for comparison** — `get_cur_eid` at the same file, line 24, *does*
  guard `if (interface >= static_cast<uint16_t>(Interface::UsEnd))`. That
  asymmetry is the bug.

### 2.2 The dispatch path (production-reachable)

The interface byte arrives over the network in the MCTP packet's private header
and travels:

1. `Validator::validate(rx, iface)` —
   `corepdk/modules/mctp-cpp/src/app/pdk-mctp-app-validator.cpp`. The guard
   reads `iface >= Interface::End`. `Interface::End == 18`, so any
   `iface ∈ [2, 17]` passes validation.
2. `Control::on_set_endpoint_id(rx, tx)` —
   `corepdk/modules/mctp-cpp/src/platforms/x86/pdk-mctp-platforms-control.cpp:50`.
   Dispatched from `Control::process` (line 33, `case Cmd::SetEpId`).
3. Line 61 unconditionally calls
   `platforms::set_cur_eid(_router, get_packet_interface(rx), crx.data[1])`
   for `SetEidNormal` and `SetEidForced` sub-commands.

### 2.3 The validator gap

```
cur_eid sized by:   UsEnd  == 2     (Interface::UsI2c, Interface::UsUsb)
validate() guard:   >= End == 18    (the gap spans 2..17)
```

Any Control SetEpId Request with `packet_interface ∈ [2, 17]` reaches
`set_cur_eid()` with an out-of-bounds index, and `std::array::at` trips the
bounds check.

### 2.4 Runtime behaviour

OpenSMA builds with `-fno-exceptions -fno-rtti`, so the `std::out_of_range`
exception path in libc++/libstdc++ degrades to `abort()`. A successful exploit
therefore yields a denial-of-service / device reset, not silent memory
corruption. `verification/ctest/f1/` demonstrates this empirically with ASan.

---

## 3. The ESBMC harness

The harness lives at `verification/harnesses/mctp_dispatch_harness.cpp`.

### 3.1 Entry point

ESBMC starts at `main()`. We construct a minimal but real `Validator` and a
`Control` thin-subclass, then invoke the production code paths.

### 3.2 Nondeterministic inputs

- `iface_val` — the attacker-controlled interface byte.
- `rx.msg[4]` (the EID payload) — left nondet; the bug is independent of it
  but leaving it free keeps the proof obligation maximal.

### 3.3 Assumptions (`__ESBMC_assume`)

A single assumption constrains the interface to the gap:

```cpp
__ESBMC_assume(iface_val >= UsEnd && iface_val < IfEnd);
```

This is sound because the claim is precisely: *for some
`iface ∈ [UsEnd, End)`, the program OOBs.* Anything tighter would still find
the bug but weaken the witness; anything broader would let `validate()`
legitimately reject the packet and obscure the path.

### 3.4 Triggering the vulnerable path

Every other packet field is set to literal values that satisfy
`Validator::validate()`:

```cpp
rx.hdr.hdr_ver   = 0x1;                                       // header version guard
rx.hdr.dst_eid   = NULL_EID;                                  // eid_ok branch
rx.hdr.som = rx.hdr.eom = rx.hdr.tag_owner = 1;               // SOM + EOM + Request
rx.msg[0] = static_cast<uint8_t>(MsgType::Control);           // dispatch to Control::process
rx.msg[1] = 0x80;                                              // rq=1, d=0 → PacketType::Request
rx.msg[2] = static_cast<uint8_t>(Cmd::SetEpId);               // → on_set_endpoint_id
rx.msg[3] = static_cast<uint8_t>(SetEndpoint::SetEidNormal);  // → set_cur_eid
set_packet_interface(rx, iface_val);                           // stamp interface into private header
```

### 3.5 Calling production code

```cpp
struct VerifControl : Control {
    using Control::on_set_endpoint_id;   // promote protected → public for the harness
};
...
VerifControl ctrl{};
Validator    v{ctrl.router()};

bool valid = v.validate(rx, static_cast<Interface>(iface_val));
if (valid) ctrl.on_set_endpoint_id(rx, tx);
```

`VerifControl` is the *only* code change relative to production — it adds no
logic, only a visibility hoist. The `Validator` shares the `RoutingTable`
instance with `Control` so both ends of the chain operate on the same object.

---

## 4. Stubs

F-1 needs very few stubs because the dispatch chain is pure C++. ESBMC
compiles the real `pdk-mctp-app-validator.cpp`,
`pdk-mctp-platforms-control.cpp`, `pdk-mctp-platforms-router-plat.cpp`, and
`pdk-mctp-platforms-packet-plat.cpp` directly.

Stubs live under `verification/stubs/`:

- `pdk-cmn-flowcontrol.h` — replaces firmware logging / flow-control macros
  (`PDK_LOG`, etc.) with no-ops so the harness does not drag in MCU-side
  dependencies.
- `cstring`, `algorithm`, `bit`, **`array`** — thin libc++ overlays. `bit` is
  gated on `ESBMC_BIT` so it yields to ESBMC 8.3.0's bundled `<bit>` (which
  already fixes esbmc#4191 / esbmc#4247). `array` restores the explicit
  per-element bounds assertion in `std::array::at` and `operator[]` that
  ESBMC 8.3.0 dropped (see §6.2).

The single nondeterministic primitive is declared by the harness itself:

```cpp
extern "C" uint8_t nondet_u8();
```

ESBMC provides `nondet_*` implementations intrinsically — no `.cpp` stub file
is required.

**Minimality argument.** The only production behaviour the proof depends on is
(1) that `Validator::validate` admits some `iface ∈ [UsEnd, End)`, and (2)
that `Control::on_set_endpoint_id` forwards that byte to `set_cur_eid`. Both
are linked from source. The harness over-approximates nothing outside that
chain, so the counterexample is sound for the real program.

---

## 5. Property

ESBMC checks a bounds assertion on the array access:

- With **ESBMC 8.2.x**: the explicit `__ESBMC_assert(n < N, "Index out of
  bounds")` inside the bundled `<array>::at` overlay.
- With **ESBMC 8.3.0+**: the same assertion, restored via
  `verification/stubs/array` (this repo's shim, see §6.2).

When `iface_val == 2`, the implicit conversion to `size_type` produces
`n = 2`; the assertion `n < 2` fails, and ESBMC emits *"Index out of bounds"*
with the file/line of the failing `at`. The matching SV-COMP-style property is
reachability of the assertion-fail location.

`--memory-leak-check`, `--overflow-check`, `--unsigned-overflow-check`,
`--nan-check` are also active so any other defect along the path would surface
— none does, which keeps the counterexample focused on the OOB.

---

## 6. Running ESBMC

### 6.1 Command line (verbatim from `verification/Makefile`, target `mctp_dispatch`)

```bash
ESBMC=$HOME/esbmc/build/src/esbmc/esbmc
REPO=$PWD                         # run from the NVIDIA-OpenSMA root

$ESBMC \
  --std c++20 \
  --memory-leak-check --overflow-check --unsigned-overflow-check --nan-check \
  --unwind 4 \
  -I$REPO/verification/stubs \
  -I$REPO/corepdk/modules/mctp-cpp/src \
  -I$REPO/corepdk/modules/mctp-cpp/src/platforms/x86 \
  $REPO/verification/harnesses/mctp_dispatch_harness.cpp \
  $REPO/corepdk/modules/mctp-cpp/src/app/pdk-mctp-app-validator.cpp \
  $REPO/corepdk/modules/mctp-cpp/src/platforms/x86/pdk-mctp-platforms-control.cpp \
  $REPO/corepdk/modules/mctp-cpp/src/platforms/x86/pdk-mctp-platforms-router-plat.cpp \
  $REPO/corepdk/modules/mctp-cpp/src/platforms/x86/pdk-mctp-platforms-packet-plat.cpp \
  2>&1 | tee /tmp/mctp_dispatch.log
```

Or, equivalently from the repository root:

```bash
make -C verification mctp_dispatch     # writes verification/results/mctp_dispatch.log
```

### 6.2 ESBMC-version notes

The report's results were produced with **ESBMC 8.2.0** (Bitwuzla 0.8.2). The
re-run for this guide used **ESBMC 8.3.0** (Bitwuzla 0.9.0) and ships two
tweaks under `verification/stubs/` so the same harness reproduces both
verdicts:

1. **`verification/stubs/bit`** — the overlay previously redefined
   `std::bit_cast`. ESBMC 8.3.0's bundled `<bit>` already ships the
   const-correct, provenance-preserving implementation that the overlay
   existed to provide (commits for esbmc#4191, esbmc#4247). The overlay is now
   gated on `#ifndef ESBMC_BIT` so it cleanly yields when the bundled header
   has already been included. Backward-compatible with 8.2.x.
2. **`verification/stubs/array`** — ESBMC 8.3.0 dropped the explicit
   `__ESBMC_assert(index < N, "Index out of bounds")` from `std::array::at`
   and `operator[]` (commit `6121c894ed`, "[cpp] make std::array accessors
   constexpr") because `__ESBMC_assert` is not a constant expression.
   The commit message states "out-of-bounds access is still reported via
   ESBMC's underlying C-array bounds check" — but this is only true when the
   array is the parent object. When `std::array<uint8_t, 2>` is a *member* of
   a larger struct (as `cur_eid` is inside `RoutingTable::Routing`), the
   dereference check sees the access fall within the parent footprint and
   does not fire. The overlay restores the explicit per-element assertion on
   the non-const accessors. Backward-compatible with 8.2.x (where the
   bundled `<array>` defines `ESBMC_ARRAY`/equivalent guard).

Without these two stubs, on ESBMC 8.3.0 the bundled `<bit>` collides with the
overlay (parse error), or — once the parse error is past — the dispatch
harness returns a spurious `VERIFICATION SUCCESSFUL`. The minimal reproducer
in §8 still triggers an OOB on 8.3.0 even *without* the `array` stub, because
its parent struct has no fields after `cur_eid` and the C-array check fires
naturally.

### 6.3 Why these flags

| Flag                                                                       | Purpose |
|----------------------------------------------------------------------------|---------|
| `--std c++20`                                                              | Harness uses `std::bit_cast`, designated initialisers, and `<bit>`. |
| `--memory-leak-check --overflow-check --unsigned-overflow-check --nan-check` | OpenSMA's standard language-level safety profile. Keeps the proof obligation broad without obscuring the F-1 counterexample. |
| `--unwind 4`                                                               | Sufficient — `validate()` contains a fixed-bound loop that converges in ≤ 4 iterations on this packet, and there is no other loop on the path. ESBMC reports an unwinding assertion failure if 4 is too few. |

You do **not** need `--no-unwinding-assertions`. Keep them on; the verdict is
a bounds-check violation, not loop truncation.

### 6.4 Recommended timeout

Empirically < 1 s wall time on a 2024 MacBook Pro. CI-style:

```bash
timeout 300 make -C verification mctp_dispatch      # GNU coreutils timeout
gtimeout 300 make -C verification mctp_dispatch     # macOS Homebrew coreutils
```

### 6.5 If verification initially fails (unexpected SUCCESSFUL)

Symptom → most likely cause → fix:

- **`VERIFICATION SUCCESSFUL`** with ESBMC 8.3.0+ and no
  `verification/stubs/array` present — see §6.2 item 2. Add the overlay and
  rerun.
- **`VERIFICATION SUCCESSFUL`** otherwise — you are on a post-fix commit.
  Check `git log --oneline -- corepdk/modules/mctp-cpp/src/platforms/x86/pdk-mctp-platforms-router-plat.cpp`
  and check out a commit before the upstream guard landed.
- **Parse error in `verification/stubs/bit` about `reinterpret_cast` casting
  away qualifiers, or about ambiguous `bit_cast`** — see §6.2 item 1. Make
  sure the overlay is gated on `#ifndef ESBMC_BIT`.
- **Compile error in `pdk-mctp-platforms-control.cpp` about `Cmd` switch
  normalisation** — your ESBMC predates PR #4235. Use 8.2.0 or newer.
- **`Unwinding loop … iteration N` followed by SUCCESSFUL** with
  `--no-unwinding-assertions` — do *not* use that flag; it can mask reachable
  bugs in truncated loops. Raise `--unwind` instead.

---

## 7. Interpreting the result

A successful reproduction looks like this (ESBMC 8.3.0 re-run on
`vr_sma`, 2026-05-25):

```
Generated 445 VCC(s), 13 remaining after simplification (110 assignments)
Solving with solver Bitwuzla 0.9.0
Runtime decision procedure: 0.002s
Building error trace

[Counterexample]

State 5  ...mctp_dispatch_harness.cpp line 63  function main
  iface_val = 2 (00000010)

State 7  ...mctp_dispatch_harness.cpp line 84  function main
  *return_value$_operator[]$18 = { .priv={...}, .hdr={...},
    .msg={ 0, 128, 1, 0, ..., 0 } }

State 8  ...pdk-mctp-platforms-packet-plat.cpp line 43
         function set_packet_interface
  pkt::0->priv.packet_interface = { .priv=2, ... }

State 9  ...mctp_dispatch_harness.cpp line 88  function main
  valid = 1

State 10 ...verification/stubs/array line 67  function at
Violated property:
  file .../verification/stubs/array line 67  function at
  Index out of bounds
  index::0 < 2

VERIFICATION FAILED
ESBMC version 8.3.0 64-bit aarch64 macos
```

The original 8.2.0 trace from `verification/results/mctp_dispatch.log` is
byte-identical apart from the array-overlay source path (the bundled
`<array>` lived under `/var/folders/.../esbmc-cpp-headers-…/array` in 8.2.0
and under `verification/stubs/array` in the 8.3.0 re-run) and the solver
version line. The VCC count (`16 remaining` vs `13 remaining`) differs because
ESBMC 8.3.0 simplifies more aggressively; both runs leave the F-1 assertion
unsimplified and find it violated.

How to read this:

- **State 5** — Bitwuzla picks the smallest gap value, `iface_val = 2`, as the
  witness. Any value in `[2, 17]` would also work; the solver minimises.
- **State 7** — the constructed `Packet`: `msg[0..3] = { 0, 128, 1, 0 }` =
  `{ Control, 0x80=rq, SetEpId, SetEidNormal }`. A real, on-the-wire-valid
  Control SetEpId Request.
- **State 8** — `set_packet_interface` stamps `iface_val=2` into the private
  header. The packet now looks identical to one that arrived over the network
  with an out-of-range interface.
- **State 9** — `valid = 1`: `Validator::validate()` accepted the packet.
  **This is the validator gap.**
- **State 10** — `Control::on_set_endpoint_id → set_cur_eid → cur_eid.at(2)`
  on a size-2 array. The bounds assertion in `std::array::at` fails:
  `index 2 < 2` is false. ESBMC reports the violated property and exits
  FAILED.

### 7.1 What this proves

The trace is reachability-complete from `main()` through *production*
`validate()` and *production* `on_set_endpoint_id()` to the OOB write. The
only non-production code in the path is `VerifControl`'s visibility hoist,
which adds zero behaviour. F-1 is a formally proven, production-reachable
vulnerability — not a static-analysis warning.

### 7.2 Empirically confirming the runtime effect

`verification/ctest/f1/` contains a 26-line standalone demonstrator that
mirrors the failure under `-fno-exceptions -fno-rtti` and `-fsanitize=address`.
From the repo root:

```bash
cd verification/ctest/f1
cmake -B build && cmake --build build
./build/test_case_1; echo "exit=$?"
```

Expected: the program aborts with `SIGABRT` (exit 134). ASan additionally
prints a *stack-buffer-overflow* report at the `cur_eid.at(2) = 0xAB` line.
This confirms that the ESBMC-found path produces immediate program
termination, not silent corruption, under the real toolchain.

---

## 8. Minimal standalone reproducer

If you do not want to clone OpenSMA at all, the following file is sufficient
to exhibit the underlying defect class on the same `std::array<uint8_t, 2>`
shape ESBMC checks in the full proof. Save as `f1_min.cpp`:

```cpp
// f1_min.cpp — minimal standalone witness of the F-1 pattern.
//
// Mirrors corepdk/.../pdk-mctp-platforms-router-plat.cpp:34 with:
//   - the same array size (UsEnd == 2)
//   - the same lack of a guard
//   - the same attacker-controlled interface range modelled by validate()
// Expected verdict: VERIFICATION FAILED — "Index out of bounds".
#include <array>
#include <cstdint>

extern "C" uint8_t nondet_u8();

static constexpr uint8_t UsEnd = 2;   // cur_eid size
static constexpr uint8_t IfEnd = 18;  // Validator::End

struct RoutingTable { std::array<uint8_t, UsEnd> cur_eid{}; };

static void set_cur_eid(RoutingTable& rt, uint8_t iface, uint8_t eid) {
    rt.cur_eid.at(iface) = eid;       // unguarded — the bug
}

static bool validate(uint8_t iface) { return iface < IfEnd; }

int main() {
    RoutingTable rt;
    uint8_t iface = nondet_u8();
    uint8_t eid   = nondet_u8();
    __ESBMC_assume(iface >= UsEnd && iface < IfEnd);
    if (validate(iface)) set_cur_eid(rt, iface, eid);
    return 0;
}
```

Verify on ESBMC 8.2.x or 8.3.0+:

```bash
$ESBMC --std c++20 --memory-leak-check --overflow-check \
       --unsigned-overflow-check --nan-check --unwind 4 \
       f1_min.cpp
```

Re-run (ESBMC 8.3.0, 2026-05-25), VERIFIED:

```
Generated 7 VCC(s), 3 remaining after simplification (11 assignments)
Solving with solver Bitwuzla 0.9.0
[Counterexample]
  iface = 17 (00010001)
Violated property:
  file f1_min.cpp line 13  function set_cur_eid
  dereference failure: Access to object out of bounds
  CWE: CWE-125, CWE-787, CWE-823
VERIFICATION FAILED
```

(In this minimal case the C-array dereference check fires directly because
`cur_eid` is the sole field in `RoutingTable` and the access reaches past the
parent. The error message therefore says "dereference failure" rather than
"Index out of bounds". Add `-I$REPO/verification/stubs` to get the precise
"Index out of bounds" message via the array overlay if you want consistency
with the production-path log.)

**Why this is sufficient.** The full dispatch proof (§§3–7) shows that
*production* code reaches `set_cur_eid` with `iface ∈ [UsEnd, End)`. Given
that, the only remaining question — "does `std::array<uint8_t, 2>::at(i)`
actually trip the bounds check for `i ≥ 2`?" — is answered by this minimal
program. The two proofs together cover reachability + violation; either one
alone is weaker. The repository's `verification/ctest/f1/array_oob.cpp` is
essentially this minimal reproducer with an additional `printf` to
characterise the runtime side effect.

---

## 9. Cross-references

- Bug report: `verification/REPORT.md` § "F-1 — `set_cur_eid()` lacks bounds
  check" (lines 142–224).
- Harness: `verification/harnesses/mctp_dispatch_harness.cpp`.
- Stubs: `verification/stubs/pdk-cmn-flowcontrol.h`,
  `verification/stubs/bit`, `verification/stubs/array` (the last two are
  required for ESBMC 8.3.0+; see §6.2).
- Runtime demonstrator: `verification/ctest/f1/array_oob.cpp`,
  `verification/ctest/f1/CMakeLists.txt`.
- ESBMC build target: `make -C verification mctp_dispatch`.
- Upstream fix:
  <https://github.com/NVIDIA/OpenSMA/issues/1#issuecomment-4417902007>
  (mirrors the `interface >= UsEnd` guard into `set_cur_eid`).
- Validation log of this guide:
  `verification/results/mctp_dispatch.log` (regenerated by the command
  above; matches the report verdict).

Reproduction time on a 2024 laptop, from a built ESBMC and a fresh checkout:
< 30 s.
