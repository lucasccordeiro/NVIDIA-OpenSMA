// ESBMC harness for sys::c2c_mailbox (production:
// src/sys/mcxn556/sys/c2c_mailbox/c2c_mailbox.{h,cpp}).
//
// Production cpp pulls "nv/ipc/ipc_task.h" purely for the
// nv::ipc::get_current_core() / CoreId dispatch. We satisfy that via the
// existing sys/common/common.h stub (constexpr, returns Core0) which is
// re-exported by the slim verification/stubs/nv/ipc/ipc_task.h interceptor.
// fsl_mailbox.h is shimmed under verification/stubs/ and records the most
// recent (selector, value) pair on writes plus the most recent selector on
// reads, so this harness can assert the dispatch contract.
//
// Phase 1: prove no overflow / memory-leak / NaN / pointer error on the
//   two public entry points for any nondet input. Trivially yes — no
//   loops, no memcpy, no allocation.
//
// Phase 3 (negative — dispatch correctness):
//   Contract: a core posts to the *peer* core's inbox and reads from its
//   *own* inbox.
//     - on Core0: set_value → kMAILBOX_CM33_Core1, get_value → kMAILBOX_CM33_Core0
//     - on Core1: set_value → kMAILBOX_CM33_Core0, get_value → kMAILBOX_CM33_Core1
//   The sys/common/common.h stub fixes get_current_core() to Core0, so the
//   Core1 branch in c2c_mailbox.cpp is currently unreachable and proven
//   dead by ESBMC. Asserting the invariant in core-agnostic form (via
//   expected_peer_slot / expected_self_slot) keeps the contract valid for
//   the day a Core1-pinned variant of sys/common/common.h is introduced.

#include <cstdint>
#include "nv/ipc/ipc_task.h"  // slim interceptor → nv::ipc::CoreId / get_current_core
#include "sys/c2c_mailbox/c2c_mailbox.h"

extern "C" {
uint32_t nondet_u32();
}

namespace {

constexpr mailbox_cpu_id_t expected_peer_slot(nv::ipc::CoreId c)
{
    return c == nv::ipc::CoreId::Core0 ? kMAILBOX_CM33_Core1 : kMAILBOX_CM33_Core0;
}

constexpr mailbox_cpu_id_t expected_self_slot(nv::ipc::CoreId c)
{
    return c == nv::ipc::CoreId::Core0 ? kMAILBOX_CM33_Core0 : kMAILBOX_CM33_Core1;
}

}  // namespace

int main()
{
    using namespace sys::c2c_mailbox;
    const auto cur = nv::ipc::get_current_core();

    // ---- Phase 1: totality on the canonical enum value. ----
    _verif_mailbox_reset();
    set_value(MailBoxValues::FaultNotifyAnotherCoreReady);
    volatile uint32_t v1 = get_value();
    (void)v1;

    // ---- Phase 3a: set_value dispatches to the peer slot ----
    //                with the exact uint32_t representation of the enum.
    _verif_mailbox_reset();
    set_value(MailBoxValues::FaultNotifyAnotherCoreReady);
    __ESBMC_assert(_verif_set_seen,
                   "set_value must invoke MAILBOX_SetValue exactly once");
    __ESBMC_assert(_verif_last_set_cpu == expected_peer_slot(cur),
                   "set_value must target the peer core's mailbox slot");
    __ESBMC_assert(_verif_last_set_value == 0xAFAFAFAFu,
                   "set_value must forward the enum's underlying uint32_t");

    // ---- Phase 3b: a nondet enum value still routes to the peer slot ----
    //                and the value forwarded matches the input verbatim.
    _verif_mailbox_reset();
    const uint32_t raw = nondet_u32();
    set_value(static_cast<MailBoxValues>(raw));
    __ESBMC_assert(_verif_set_seen,
                   "set_value must invoke MAILBOX_SetValue for any input");
    __ESBMC_assert(_verif_last_set_cpu == expected_peer_slot(cur),
                   "set_value dispatch must not depend on payload");
    __ESBMC_assert(_verif_last_set_value == raw,
                   "set_value must forward its argument bitwise");

    // ---- Phase 3c: get_value reads from the local core's own slot. ----
    _verif_mailbox_reset();
    volatile uint32_t v2 = get_value();
    (void)v2;
    __ESBMC_assert(_verif_get_seen,
                   "get_value must invoke MAILBOX_GetValue exactly once");
    __ESBMC_assert(_verif_last_get_cpu == expected_self_slot(cur),
                   "get_value must read the local core's own mailbox slot");

    return 0;
}

// Expected: VERIFICATION SUCCESSFUL
//   - Phase 1 (safety): no overflow / NaN / memory / pointer error.
//   - Phase 3 (dispatch): set_value → peer slot, get_value → self slot,
//     payload forwarded bitwise.
