// F-7 failure-mode demonstrator: PortRecoveryPayload no rollback on validation failure.
//
// Production: on_dev_cfg_submit_ErrorInjectionPayload (nsm_type_5.cpp lines 1095-1106):
//   1. memcpy incoming data → type5_data.portRecoveryResp     (write-before-validate)
//   2. validate; if validation fails → fill_error_packet; return  (no rollback of step 1)
// Contrast with the FatalError path (lines 1077-1086) which resets the stored
// field before returning: type5_data.fatalerrorResp = FatalErrorPayload{}.
//
// Expected runtime behaviour: assert fires because portRecoveryResp retains
// the dirty incoming data after a failed validation.

#include <cassert>
#include <cstdint>
#include <cstring>

extern "C" {
unsigned char __VERIFIER_nondet_uchar(void);
unsigned int  __VERIFIER_nondet_uint(void);
void          __VERIFIER_assume(int cond);
}

struct [[gnu::packed]] PortRecoveryPayload {
    uint32_t offset{0};
    uint16_t error_injection_id{0};
    uint16_t error_type{0};
    uint8_t  l1_recovery_bitmap{0};
    uint8_t  l2_recovery_bitmap{0};
    uint8_t  l3_recovery_bitmap{0};
    uint8_t  reserved_1{0};
};

static bool is_zero_initialised(const PortRecoveryPayload& p)
{
    return p.offset == 0 && p.error_injection_id == 0 && p.error_type == 0
        && p.l1_recovery_bitmap == 0 && p.l2_recovery_bitmap == 0
        && p.l3_recovery_bitmap == 0 && p.reserved_1 == 0;
}

int main()
{
    PortRecoveryPayload stored{};

    PortRecoveryPayload incoming{};
    incoming.offset             = static_cast<uint32_t>(__VERIFIER_nondet_uchar());
    incoming.error_injection_id = static_cast<uint16_t>(__VERIFIER_nondet_uchar());
    incoming.error_type         = static_cast<uint16_t>(__VERIFIER_nondet_uchar());
    incoming.l1_recovery_bitmap = __VERIFIER_nondet_uchar();
    incoming.l2_recovery_bitmap = __VERIFIER_nondet_uchar();
    incoming.l3_recovery_bitmap = __VERIFIER_nondet_uchar();
    incoming.reserved_1         = __VERIFIER_nondet_uchar();

    __VERIFIER_assume(incoming.offset != 0);  // ensure incoming is observably dirty

    // Step 1: write-before-validate (production memcpy).
    memcpy(&stored, &incoming, sizeof(PortRecoveryPayload));

    // Step 2: validation — nondeterministic; constrained to the failure path.
    const bool valid = static_cast<bool>(__VERIFIER_nondet_uint() % 2);
    __VERIFIER_assume(!valid);

    if (!valid) {
        // BUG: production returns here without resetting stored.
        // Post-condition: stored must be zero-initialised on validation failure.
        assert(is_zero_initialised(stored) &&
               "F-7: portRecoveryResp not zeroed after validation failure (no rollback)");
        return 0;
    }

    return 0;
}
