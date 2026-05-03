// ESBMC system-level harness for F-6: reachability proof via
// Nsm::process_device_configuration() — production source compiled as-is.
//
// Finding: Nsm::on_dev_cfg_set_errorInjectionMode (nsm_type_5.cpp:777)
//   stores nrx.data[0] into type5_data.errorInjectionModeResponse.mode
//   without validating the byte against {Disable=0, Enable=1}.
//
// This harness calls the REAL Nsm::process_device_configuration() (production
// code, not an inline copy) with a crafted NSM DeviceConfiguration packet
// whose mode byte is nondet.
//
// VerifNsm is a thin subclass that promotes the protected method to public,
// following the same pattern as mctp_dispatch_harness.cpp (VerifControl).
//
// Expected: VERIFICATION FAILED — CEX traces nrx.data[0] through real
// compiled code to the unchecked assignment, proving the bug is reachable
// from the production dispatch path.

#include "nv/mctp/nsm.h"
#include "nv/mctp/nsm_type_5.cpp"

using namespace nv;
using namespace nv::mctp;

extern "C" uint8_t nondet_u8();

// Thin subclass exposing the protected process_device_configuration.
// type5_data is protected in Nsm, so expose it via a const getter.
struct VerifNsm : Nsm {
    explicit VerifNsm(Control& ctl) : Nsm(ctl) {}
    using Nsm::process_device_configuration;
    const NsmDevCfgPersistentData& get_type5_data() const { return type5_data; }
};

int main()
{
    // Nsm requires a Control& but nsm_type_5.cpp never dereferences _ctl.
    // We satisfy the reference via a minimal stub object.
    Control ctl_stub{};
    VerifNsm nsm{ctl_stub};

    // Build an NSM DeviceConfiguration / SetErrorInjectionMode request.
    Packet rx{}, tx{};
    auto& nrx = NsmPktReq::from(rx);
    nrx.nv_msg_type = NsmMsgType::DeviceConfiguration;
    nrx.set_dev_cfg_code(NsmDevCfgCmdCode::SetErrorInjectionMode);

    // Nondet mode byte — represents the attacker-controlled nrx.data[0].
    nrx.data[0] = nondet_u8();

    nsm.process_device_configuration(rx, tx);

    // Post-condition: mode must be Disable (0x00) or Enable (0x01).
    const auto mode = nsm.get_type5_data().errorInjectionModeResponse.mode;
    __ESBMC_assert(
        mode == NsmDevCfgEnablingMode::Disable
     || mode == NsmDevCfgEnablingMode::Enable,
        "F-6 system: errorInjectionModeResponse.mode must be Disable or Enable");

    return 0;
}
