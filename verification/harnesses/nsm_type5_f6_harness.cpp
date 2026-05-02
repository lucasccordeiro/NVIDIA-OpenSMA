// ESBMC negative harness for F-6: SetErrorInjectionMode accepts arbitrary uint8_t mode.
//
// Finding: on_dev_cfg_set_errorInjectionMode stores nrx.data[0] directly into
// type5_data.errorInjectionModeResponse.mode without checking that the byte is
// 0x00 (Disable) or 0x01 (Enable).
//
// This harness inlines the assignment logic verbatim and asserts the post-condition:
//   mode ∈ {NsmDevCfgEnablingMode::Disable, NsmDevCfgEnablingMode::Enable}
//
// When the Disable branch is NOT taken (i.e., mode != Disable), the code falls
// through unconditionally to the store:
//   type5_data.errorInjectionModeResponse.mode = nrx.data[0];
// so any mode value other than 0x00 or 0x01 (e.g., 0x02) is accepted.
//
// Expected: VERIFICATION FAILED — assertion "mode in {Disable, Enable}" fails
// for any nondet mode value ∉ {0x00, 0x01}.

#include <cstdint>

extern "C" { uint8_t nondet_u8(); }

// Verbatim from nsm_type_5.h
enum NsmDevCfgEnablingMode : uint8_t
{
    Disable = 0x00,
    Enable  = 0x01,
};

struct NsmDevCfgErrorInjectionModeResponse
{
    uint8_t mode;
    NsmDevCfgErrorInjectionModeResponse() : mode{NsmDevCfgEnablingMode::Disable} {}
};

int main()
{
    NsmDevCfgErrorInjectionModeResponse errorInjectionModeResponse{};

    // Nondet mode byte — represents nrx.data[0] from the NSM request.
    uint8_t request_mode = nondet_u8();

    // Production logic from on_dev_cfg_set_errorInjectionMode (lines 795-801):
    //   if (nrx.data[0] == NsmDevCfgEnablingMode::Disable) {
    //       if (!type5_data.isCurrentErrorInjectionBitmaskCleared()) {
    //           fill_error_packet(Ccode::ErrorGeneral, rx, tx);
    //           return;
    //       }
    //   }
    //   type5_data.errorInjectionModeResponse.mode = nrx.data[0];  // <-- no range check
    //
    // Simplified: assume the bitmask is cleared (so the Disable path does not return early),
    // which is the most permissive assumption — it allows all mode values to reach the store.
    const bool bitmask_cleared = true;  // __ESBMC_assume best case

    if (request_mode == NsmDevCfgEnablingMode::Disable) {
        if (!bitmask_cleared) {
            // Would return error — skip this path
            return 0;
        }
    }
    // Unconditional store with no range validation
    errorInjectionModeResponse.mode = request_mode;

    // Post-condition: mode must be Disable (0x00) or Enable (0x01).
    // This fails when request_mode is any other uint8_t value.
    __ESBMC_assert(
        errorInjectionModeResponse.mode == NsmDevCfgEnablingMode::Disable
            || errorInjectionModeResponse.mode == NsmDevCfgEnablingMode::Enable,
        "F-6: errorInjectionModeResponse.mode must be Disable or Enable");

    return 0;
}
