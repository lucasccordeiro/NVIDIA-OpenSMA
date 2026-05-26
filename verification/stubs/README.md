# `verification/stubs/` — provenance and rationale

This directory holds the headers ESBMC sees instead of (or in addition to) the
real production headers when running any harness under `verification/`. Two
kinds of file live here, with very different origins. Knowing which kind you
are looking at matters: the standard-library overlays are reusable outside
this repository; the firmware-specific stubs are not.

All files were authored by hand — by the verification team, working from the
cited ESBMC commit/issue or NVIDIA firmware header. None are LLM-generated
boilerplate. Each header carries a top-of-file comment explaining what it
shadows and why.

The include path used by every harness is `-I verification/stubs`. Where a
stub takes a `*_CONFIG_H` form (e.g. `ssif_config.h`), it is supplied via
`-DNV_IPC_CONFIG_H='"…"'` from the harness `Makefile`.

---

## Group 1 — C++ standard-library overlays

Thin shims over ESBMC's bundled libc++ headers. Each closes a *specific* gap
in a *specific* ESBMC release, with the upstream issue cited inline. As issues
are fixed upstream the corresponding overlay is removed (see the "removed"
column in `verification/README.md` and `verification/REPORT.md`). What remains
here is the minimum still needed against ESBMC 8.3.0.

| File | Shadows | Purpose | Upstream | Status on 8.3.0 |
|---|---|---|---|---|
| `array` | `<array>` | Restores the explicit `__ESBMC_assert(index < N, "Index out of bounds")` on `std::array::at` and `operator[]` that ESBMC 8.3.0 dropped in commit `6121c894ed` ("[cpp] make std::array accessors constexpr"). The commit assumed ESBMC's underlying C-array dereference check still fires; it does, when the array is the parent object, but **not** when the array is a member of a larger struct — the access falls within the parent footprint and the check is suppressed. F-1's witness path (`routing_table.ec.cur_eid.at(iface)`) is exactly this aliasing case. | esbmc#4269 (the constexpr add) — regression filed separately; see `F1_REPRO.md §6.2` | **Required** for F-1 reproducibility |
| `algorithm` | `<algorithm>` (`#include_next`) | Injects `std::copy_n` (delegates to bundled `std::copy`) which the bundled `<algorithm>` does not ship. Used by ssif and mctp. | esbmc#4251 (the related `std::clamp` gap, fixed upstream) | Optional; convenience |
| `bit` | `<bit>` (gated on `#ifndef ESBMC_BIT`) | Historical full replacement of `std::bit_cast` for pointer-to-pointer const-source casts with provenance preservation. ESBMC 8.3.0's bundled `<bit>` already ships the const-correct, provenance-preserving implementation, so the overlay now cleanly yields when the bundled header is included first. Kept for backward compatibility with ESBMC 8.2.x. | esbmc#4191, esbmc#4247 (both fixed) | Inert on 8.3.0; live on 8.2.x |
| `cstring` | `<cstring>` | Pulls `<string.h>` and re-exports `memcpy`/`memmove`/`memset`/`memcmp`/`strlen`/… into `std::` so production headers that write `std::memcpy(...)` compile. ESBMC's bundled `<cstring>` only exposes C-linkage names in the global namespace. | (no issue filed; trivial gap-filler) | Required by any TU that uses qualified `std::memcpy` etc. |

These four files are **generic**: any ESBMC user hitting the same version
combination could drop them into their own `-I` path and they would work.
They have no NVIDIA-specific knowledge baked in.

Historical entries (now removed because the upstream fix landed):
`span`, `type_traits`, `tuple` workarounds, `array` aggregate fixes — see
`verification/README.md` for the per-issue ledger.

---

## Group 2 — NVIDIA firmware-specific stubs

These shadow real headers from this repository (or from the NXP MCUXpresso
SDK / mbedTLS / FreeRTOS) that the production firmware depends on but that
ESBMC cannot meaningfully execute: peripheral I/O, RTOS tasks/queues/timers,
mailbox hardware, logging, cryptography, flash drivers, etc. Each stub
provides the **minimum** type and function surface needed by the call sites
the harness actually reaches, with side effects either replaced by
`nondet_*` returns or recorded into `_verif_*` shadow state for the harness
to assert on.

Authoring approach for every file in this group:

1. Identify the smallest set of symbols from the real header that the
   harness's reachable call graph actually touches.
2. Reproduce those symbols' **signatures** verbatim against the production
   source under `corepdk/`, `src/nv/`, or `src/sys/`, so the compiled
   harness links and overload resolution behaves identically to production.
3. Replace bodies with verification semantics — `nondet_*` returns for
   reads, no-ops for writes, `__ESBMC_assert` for invariants, or
   shadow-state recording where the harness needs to observe a side
   effect.
4. Keep the header self-contained and dependency-minimal. Where two stubs
   share a type (e.g. `nv::ipc::CoreId`), the canonical definition lives
   in one stub and the others `#include` it.

| Subtree / file | Mirrors | Reason for stubbing |
|---|---|---|
| `pdk-cmn-flowcontrol.h` | `corepdk/modules/cmn/include/pdk/cmn/flowcontrol.h` | Real header drags in `pdk/cmn/log/log.h` and an Ada-exported exit symbol. Stub routes the assert to `__ESBMC_assert`. |
| `pdk/cmn/log/log.h` | `corepdk/.../log/log.h` | No-op logging. |
| `mbedtls/ctr_drbg.h` | mbedTLS SDK header | Opaque context + three function decls only — mbedTLS internals are out of scope. |
| `fsl_mailbox.h` | NXP MCUXpresso SDK `devices/MCXN947/drivers/fsl_mailbox.h` | Provides the four C2C mailbox symbols (`MAILBOX`, two `kMAILBOX_CM33_Core*` selectors, `MAILBOX_SetValue`, `MAILBOX_GetValue`) with shadow `_verif_last_set_*` state so c2c_mailbox harness can assert dispatch correctness. |
| `nsm_f6_config.h`, `power_manager_config.h`, `ssif_config.h` | Per-platform `NV_IPC_CONFIG_H` (e.g. `testrunner/config.h`) | Platform constants for NSM F-6, PowerManager, and ssif harnesses respectively. Selected per-harness via `-DNV_IPC_CONFIG_H='"…"'` in the harness Makefile. |
| `sys/common/common.h` | `src/sys/x86/sys/common/common.h` | Provides `nv::ipc::CoreId`. |
| `sys/{adc,dac,gpio,flash,i2c,sensor}/` | Production sys-layer device headers | Hardware-peripheral interfaces with nondet returns / shadow recording. Used by F-13 system harness and ssif/c2c_mailbox. |
| `sys/ipc/{event,mutex,queue,streambuffer,supervisor,task,timer}.h` | Production sys-layer IPC primitives | RTOS primitives replaced with no-op / nondet equivalents — verification runs each task body sequentially. |
| `nv/ipc/*` | `src/nv/ipc/*` | Same intent as `sys/ipc/*` but at the higher-level NV IPC layer (event, mutex, queue, object, streambuffer, supervisor, task, timer). |
| `nv/i2c/*` | `src/nv/i2c/*` | I2C driver, sensor, helper, error-injection, lattice-CPLD, dummy-CPLD register stubs. Used wherever an I2C device is in the harness's transitive reach. |
| `nv/mctp/*` | `src/nv/mctp/*` | MCTP driver, constants, message bitmasks, NSM type-4 and pwr-smoothing handlers, and the MCTP task. |
| `nv/logger/{common,log}.h` | `src/nv/logger/*` | Logging types and entry points; bodies are no-ops because log payloads are not part of any verification goal. |
| `nv/{flash,gpio,iox,lstp,perf_mon,ssif,telemetry,debugtoken,bootloader,ipchandler}/*`, `nv/nv.h`, `nv/bootloader.h` | Corresponding `src/nv/...` headers | Each replaces a production header whose transitive includes pull in FreeRTOS or hardware drivers; the stub keeps the type surface the harness needs and drops the rest. |

Files in Group 2 are **NVIDIA-specific** and not portable outside this
codebase.

---

## How to add a new stub

When a new harness pulls in a production header whose dependencies ESBMC
cannot compile (FreeRTOS, NXP SDK, hardware drivers):

1. Add the stub under the matching path beneath `verification/stubs/`
   (e.g. shadowing `src/nv/foo/bar.h` ⇒ `verification/stubs/nv/foo/bar.h`).
2. Open the real header in `src/` or `corepdk/` and copy the exact
   signatures of every symbol the harness's reachable call graph touches.
   Do not invent signatures — overload resolution and ADL must behave the
   same as in production.
3. Replace bodies with `nondet_*` reads, no-op writes, `__ESBMC_assert`
   invariants, or shadow `_verif_*` state as appropriate.
4. Add a top-of-file comment naming the production header you are
   shadowing, the call sites that drove the symbol selection, and any
   verification semantics that differ from the real behaviour.
5. If the stub belongs to Group 1 (standard library), cite the relevant
   ESBMC issue or commit, gate it on the bundled header's include guard so
   it cleanly yields when the upstream fix lands, and add a row to the
   table above.

When an upstream ESBMC fix lands that subsumes a Group 1 overlay, remove
the file and update both this README and the per-issue ledger in
`verification/README.md`.
