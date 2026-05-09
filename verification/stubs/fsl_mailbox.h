// ESBMC verification stub for fsl_mailbox.h.
//
// Real header lives in NXP MCUXpresso SDK
// (devices/MCXN947/drivers/fsl_mailbox.h) and is not in this repository.
// sys::c2c_mailbox uses only four symbols from it:
//   - MAILBOX (peripheral base pointer)
//   - kMAILBOX_CM33_Core0 / kMAILBOX_CM33_Core1 (mailbox slot selectors)
//   - MAILBOX_SetValue(base, cpu, value)
//   - MAILBOX_GetValue(base, cpu) -> uint32_t
//
// Modeled contract (verification semantics):
//   * MAILBOX_SetValue: records (cpu, value) into _verif_last_set_* so a
//     harness can assert dispatch correctness. The hardware write itself
//     is invisible — peers run in a separate ESBMC instance.
//   * MAILBOX_GetValue: records cpu into _verif_last_get_cpu and returns
//     nondet_u32(). Any uint32_t the peer might have written is permitted.
//
// The witness variables are exposed as inline globals so harnesses can
// read them directly. Reset them at the start of each scenario via
// _verif_mailbox_reset() to make per-call assertions independent.
#pragma once
#include <cstdint>

extern "C" {
uint32_t nondet_u32();
}

typedef struct MAILBOX_Type_
{
    uint32_t reserved;
} MAILBOX_Type;

typedef enum
{
    kMAILBOX_CM33_Core0 = 0,
    kMAILBOX_CM33_Core1 = 1,
} mailbox_cpu_id_t;

inline MAILBOX_Type _esbmc_mailbox_storage{};
#define MAILBOX (&_esbmc_mailbox_storage)

// --- Verification witnesses (not present in the real SDK) -------------
inline bool             _verif_set_seen       = false;
inline mailbox_cpu_id_t _verif_last_set_cpu   = kMAILBOX_CM33_Core0;
inline uint32_t         _verif_last_set_value = 0;
inline bool             _verif_get_seen       = false;
inline mailbox_cpu_id_t _verif_last_get_cpu   = kMAILBOX_CM33_Core0;

inline void _verif_mailbox_reset()
{
    _verif_set_seen       = false;
    _verif_last_set_cpu   = kMAILBOX_CM33_Core0;
    _verif_last_set_value = 0;
    _verif_get_seen       = false;
    _verif_last_get_cpu   = kMAILBOX_CM33_Core0;
}
// ----------------------------------------------------------------------

inline void
MAILBOX_SetValue(MAILBOX_Type* /*base*/, mailbox_cpu_id_t cpu, uint32_t value)
{
    _verif_set_seen       = true;
    _verif_last_set_cpu   = cpu;
    _verif_last_set_value = value;
}

inline uint32_t MAILBOX_GetValue(MAILBOX_Type* /*base*/, mailbox_cpu_id_t cpu)
{
    _verif_get_seen     = true;
    _verif_last_get_cpu = cpu;
    return nondet_u32();
}
