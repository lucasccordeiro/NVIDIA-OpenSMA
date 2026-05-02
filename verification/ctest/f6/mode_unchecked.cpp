// F-6 failure-mode demonstrator: SetErrorInjectionMode accepts arbitrary uint8_t mode.
//
// Production: on_dev_cfg_set_errorInjectionMode (nsm_type_5.cpp lines 795-801)
// stores nrx.data[0] directly into errorInjectionModeResponse.mode with no range
// check. Any byte value outside {0x00=Disable, 0x01=Enable} is silently accepted.
//
// Expected runtime behaviour: assert fires when request_mode ∉ {Disable, Enable}.

#include <cassert>
#include <cstdint>

extern "C" {
unsigned char __VERIFIER_nondet_uchar(void);
void          __VERIFIER_assume(int cond);
}

enum NsmDevCfgEnablingMode : uint8_t { Disable = 0x00, Enable = 0x01 };

struct NsmDevCfgErrorInjectionModeResponse {
    uint8_t mode{NsmDevCfgEnablingMode::Disable};
};

int main()
{
    NsmDevCfgErrorInjectionModeResponse resp{};

    uint8_t request_mode = __VERIFIER_nondet_uchar();
    // Constrain to values outside the valid set to exercise the bug path.
    __VERIFIER_assume(request_mode != Disable && request_mode != Enable);

    // Production logic verbatim: unconditional store, no range validation.
    resp.mode = request_mode;

    // Invariant: mode must be Disable (0x00) or Enable (0x01).
    assert((resp.mode == Disable || resp.mode == Enable) &&
           "F-6: errorInjectionModeResponse.mode outside {Disable, Enable}");
    return 0;
}
