// ESBMC system-level harness for F-7 (structural/latent):
// Nsm::process_device_configuration() compiled from production nsm_type_5.cpp.
//
// Finding: on_dev_cfg_submit_ErrorInjectionPayload (nsm_type_5.cpp:1095)
//   memcpy(&type5_data.portRecoveryResp, nrx.data, sizeof(PortRecoveryPayload))
//   BEFORE calling validatePortRecoveryErrorInjectionPayload, with no rollback
//   if validation fails.
//
// This harness calls the REAL Nsm::process_device_configuration() with a crafted
// SetErrorInjectionPayload / PortRecoveryErrors packet (OCP v2, DeviceError id,
// nondet bitmaps).  It closes gap-1 (real PortRecoveryPayload/NsmDevCfgPersistentData
// types from production headers) and gap-2 (real dispatch logic from nsm_type_5.cpp)
// that the structural harness (nsm_type5_f7_harness.cpp) inlines manually.
//
// Gap-3 (validator modelled as nondet) cannot be closed: the production
// validatePortRecoveryErrorInjectionPayload always returns true (nsm_type_5.cpp:
// 206-210; a TODO stub).  The validation-failure path is dead code in the current
// codebase, so the no-rollback bug is not currently reachable.
//
// Expected: VERIFICATION SUCCESSFUL — the dead validator-failure branch is never
// explored, confirming F-7 is structurally latent.  The standard safety checks
// (overflow, memory) cover all reachable paths through the real production code.
//
// Contrast with nsm_type5_f7_neg (structural harness): that harness stubs the
// validator as nondet, exercises the failure path, and gets VERIFICATION FAILED,
// proving the structural bug exists when the TODO validator is eventually completed.

#include "nv/mctp/nsm.h"
#include "nv/mctp/nsm_type_5.cpp"

using namespace nv;
using namespace nv::mctp;

extern "C" uint8_t nondet_u8();

// Thin subclass promoting the protected process_device_configuration and
// providing read/write access to the protected type5_data member.
struct VerifNsm : Nsm {
    explicit VerifNsm(Control& ctl) : Nsm(ctl) {}
    using Nsm::process_device_configuration;
    const NsmDevCfgPersistentData& get_type5_data() const { return type5_data; }
    NsmDevCfgPersistentData&       get_type5_data_mut()   { return type5_data; }
};

int main()
{
    Control ctl_stub{};
    VerifNsm nsm{ctl_stub};

    // Pre-condition 1: error injection mode must be enabled.
    // isErrorInjectionModeEnabled() checks mode == Enable.
    nsm.get_type5_data_mut().errorInjectionModeResponse.mode =
        NsmDevCfgEnablingMode::Enable;

    // Pre-condition 2: DeviceError (id=4) must be set in current_errors_injection_bitmask.
    // isErrorTypeEnabled(4) checks bit 4 of bitmask[0].
    nsm.get_type5_data_mut().current_errors_injection_bitmask[0] =
        static_cast<uint8_t>(1u << static_cast<uint8_t>(DeviceError));

    // Build a SetErrorInjectionPayload / PortRecoveryErrors request (OCP v2).
    Packet rx{}, tx{};

    // Outer dispatch (process_device_configuration) reads nv_msg_type and cmd_code
    // via NsmPktReq.  Both NsmPktReq and NsmPktReqV2 share the same layout through
    // cmd_code, so either view can set these fields.
    auto& nrx = NsmPktReq::from(rx);
    nrx.nv_msg_type = NsmMsgType::DeviceConfiguration;
    nrx.set_dev_cfg_code(NsmDevCfgCmdCode::SetErrorInjectionPayload);

    // Inner handler (on_dev_cfg_submit_ErrorInjectionPayload) reads the packet via
    // NsmPktReqV2.  Set the V2-only fields: ocp_version must be 2 (guarded at
    // nsm_type_5.cpp:1035), data_size must be >= sizeof(PortRecoveryPayload)=12.
    auto& nrx_v2 = NsmPktReqV2::from(rx);
    nrx_v2.ocp_version = 2;
    nrx_v2.data_size   = static_cast<uint16_t>(sizeof(PortRecoveryPayload));

    // Write ErrorInjectionPayloadHeader into nrx_v2.data[0..7]:
    //   offset             = 0            (guard at nsm_type_5.cpp:1049)
    //   error_injection_id = DeviceError  (0x04, selects the DeviceError branch)
    //   error_type         = PortRecoveryErrors (0x01, selects the F-7 sub-branch)
    const ErrorInjectionPayloadHeader ei_hdr{
        0,
        static_cast<uint16_t>(DeviceError),
        static_cast<uint16_t>(PortRecoveryErrors)
    };
    memcpy(&nrx_v2.data[0], &ei_hdr, sizeof(ei_hdr));

    // Nondet PortRecoveryPayload bitmaps at data[8..11].
    nrx_v2.data[8]  = nondet_u8();  // l1_recovery_bitmap
    nrx_v2.data[9]  = nondet_u8();  // l2_recovery_bitmap
    nrx_v2.data[10] = nondet_u8();  // l3_recovery_bitmap
    nrx_v2.data[11] = nondet_u8();  // reserved_1

    nsm.process_device_configuration(rx, tx);

    // No explicit F-7 assertion is placed here.
    //
    // The structural bug (no rollback on validation failure) requires the validator
    // to return false — which it never does in current production code.  ESBMC
    // confirms VERIFICATION SUCCESSFUL: all reachable paths are memory-safe and
    // overflow-free, and the dead failure branch is never explored.
    //
    // The structural harness (nsm_type5_f7_neg) separately proves the bug exists
    // when the validator is modelled as nondet (VERIFICATION FAILED).

    return 0;
}
