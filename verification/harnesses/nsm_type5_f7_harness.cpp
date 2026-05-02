// ESBMC negative harness for F-7: PortRecoveryPayload no rollback on validation failure.
//
// Finding: on_dev_cfg_submit_ErrorInjectionPayload (lines 1088-1106) follows this pattern:
//   1. memcpy payload into type5_data.portRecoveryResp       <-- write-before-validate
//   2. call validatePortRecoveryErrorInjectionPayload()
//   3. if (status != true) { fill_error_packet(); return; }  <-- no rollback of step 1
//
// Contrast with the FatalError path (lines 1077-1086) and the USBBridgeEmulation path
// (lines 1116-1126): both reset the stored field to a zero-initialised default on
// validation failure (e.g., type5_data.fatalerrorResp = FatalErrorPayload{}).
//
// This harness:
//   - Inlines the PortRecovery write-then-validate pattern.
//   - validatePortRecoveryErrorInjectionPayload is currently a stub (always returns true),
//     but the finding is that NO rollback exists structurally, even when the validator
//     could later be extended to return false.
//   - We model validation as returning nondet bool to exercise both outcomes.
//   - Post-condition: if validation fails, portRecoveryResp must be zero-initialised.
//
// Expected: VERIFICATION FAILED — on the validation-failure path portRecoveryResp retains
// the dirty memcpy data rather than being zeroed.

#include <cstdint>
#include <string.h>

extern "C" {
uint8_t  nondet_u8();
unsigned nondet_uint();
}

// Minimal PortRecoveryPayload (verbatim from nsm_type_5.h)
struct [[gnu::packed]] PortRecoveryPayload
{
    // ei_header: offset, error_injection_id, error_type (12 bytes)
    uint32_t offset            = 0;
    uint16_t error_injection_id = 0;
    uint16_t error_type        = 0;
    uint8_t  l1_recovery_bitmap = 0;
    uint8_t  l2_recovery_bitmap = 0;
    uint8_t  l3_recovery_bitmap = 0;
    uint8_t  reserved_1        = 0;
};

// Stub matching the production signature.
// The current production implementation always returns true (TODO comment in nsm_type_5.cpp).
// We make it nondet to cover the future case where it returns false.
static bool validatePortRecoveryErrorInjectionPayload(PortRecoveryPayload& /*payload*/)
{
    // Nondet: allows ESBMC to explore both outcomes
    return static_cast<bool>(nondet_uint() % 2);
}

// Zero-value sentinel for comparison
static bool is_zero_initialised(const PortRecoveryPayload& p)
{
    return p.offset             == 0
        && p.error_injection_id == 0
        && p.error_type         == 0
        && p.l1_recovery_bitmap == 0
        && p.l2_recovery_bitmap == 0
        && p.l3_recovery_bitmap == 0
        && p.reserved_1         == 0;
}

int main()
{
    PortRecoveryPayload portRecoveryResp{};  // starts zero-initialised

    // Simulate incoming nondet payload (nrx.data bytes)
    PortRecoveryPayload incoming{};
    incoming.offset             = static_cast<uint32_t>(nondet_u8());
    incoming.error_injection_id = static_cast<uint16_t>(nondet_u8());
    incoming.error_type         = static_cast<uint16_t>(nondet_u8());
    incoming.l1_recovery_bitmap = nondet_u8();
    incoming.l2_recovery_bitmap = nondet_u8();
    incoming.l3_recovery_bitmap = nondet_u8();
    incoming.reserved_1         = nondet_u8();

    // Production PortRecovery path (nsm_type_5.cpp lines 1095-1106):
    //   memcpy(&type5_data.portRecoveryResp, &nrx.data[0], sizeof(PortRecoveryPayload));
    //   const auto status = validatePortRecoveryErrorInjectionPayload(type5_data.portRecoveryResp);
    //   if (status != true) {
    //       fill_error_packet(Ccode::ErrorInvalidData, rx, tx);
    //       return;   // <-- no rollback (compare FatalError: type5_data.fatalerrorResp = {})
    //   }

    memcpy(&portRecoveryResp, &incoming, sizeof(PortRecoveryPayload));

    const bool valid = validatePortRecoveryErrorInjectionPayload(portRecoveryResp);

    if (!valid) {
        // BUG: production returns here without resetting portRecoveryResp.
        // Post-condition: portRecoveryResp must be zero-initialised on failure.
        __ESBMC_assert(
            is_zero_initialised(portRecoveryResp),
            "F-7: portRecoveryResp must be zero-initialised on validation failure (no rollback)");
        return 0;
    }

    return 0;
}
