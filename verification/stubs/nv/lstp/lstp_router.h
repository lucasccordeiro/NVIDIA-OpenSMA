// ESBMC verification stub for nv/lstp/lstp_router.h.
//
// The production header pulls the entire LSTP stack (queues, UART driver,
// gpio common). For ssif verification we only need:
//   - nv::lstp::LstpHdr packed struct (used inside Ssif::Packet)
//   - nv::lstp::LstpRouter::send_ipmi(...) static no-op (called by handle_rx)
//
// LstpHdr layout matches src/nv/lstp/lstp_common.h:93 verbatim.
#pragma once
#include <array>
#include <cstdint>
#include <cstddef>

#include NV_IPC_CONFIG_H  // for nv::ipc::UsbLstpMsgSize

namespace nv::lstp {

struct [[gnu::packed]] LstpHdr
{
    uint8_t channel_id;
    uint8_t cmd_status_code;  // bit 7: 0=request, 1=response
    uint8_t len_lsb;
    uint8_t len_msb;
};

class LstpRouter
{
public:
    LstpRouter() = default;

    static void send_ipmi(std::array<uint8_t, nv::ipc::UsbLstpMsgSize>& /*buffer*/,
                          size_t /*size*/)
    {
        // No-op: the buffer hand-off to USB is invisible to the harness.
    }
};

}  // namespace nv::lstp
