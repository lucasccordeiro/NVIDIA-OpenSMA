// ESBMC harness for nv::ssif::Ssif (production: src/nv/ssif/ssif.{h,cpp}).
//
// Phase 1 — language-level safety baseline (NARROW SCOPE).
//
// Drives the public entry points whose code paths do NOT route through
// `Packet::from(_buffer)` / `SmbusWriteBlock::from(i2c_buffer)`:
//   - i2c_ack_callback  → i2c_ack_bmc / i2c_ack_ara
//   - handle_tx         → reads pkt.hdr only (LstpHdr; no copy/copy_n)
//   - handle_rx         → forwards _buffer to a stubbed LstpRouter
//
// The data callback (i2c_callback → smbus_block_*) is intentionally OUT
// of scope for this Phase 1 baseline because of an ESBMC pointer-
// provenance modelling issue with `std::bit_cast<Packet*>(_buffer.data())
// + member-array offset`: ESBMC's tracker loses the parent-buffer bounds
// across the cast and reports spurious OOB at `&ssif + 1` inside the
// std::copy_n on `pkt.ipmi_data`. Same root cause as the workaround the
// c2c_mailbox commit references for `pdk-mctp-app-packet.h` (esbmc#4180
// part 1 / family). Confirmed reproducible after exhausting <bit>,
// <algorithm> shims and the launder/void-pivot variants. Tracked
// separately as the `ssif_block_data_path` follow-up; needs either an
// upstream fix or a verification-only overlay of `ssif.h` that replaces
// the bit_cast factories with reinterpret_cast at struct-decl scope.
//
// Stubs in play:
//   - ssif_config.h            → EventId / UsbLstpMsgSize / EnableLstp.
//   - sys/i2c/i2c_slave.h      → NumI2cTargetAddresses, I2cSlaveBuffer*,
//                                I2CSlaveDriver<T> (no-op bind/start).
//   - nv/lstp/lstp_router.h    → LstpHdr struct + LstpRouter::send_ipmi.
//   - nv/i2c/helper.h          → crc8 returns nondet so PEC validity is
//                                a free choice for the model checker.
//   - nv/ipc/event.h           → bits() returns Expected<Bits,Status>
//                                with a nondet bitmap (covers
//                                txrx_pending true AND false branches).
//   - nv/gpio/driver.h         → no-op write, nondet read.
//   - <algorithm>              → adds std::copy_n via shim.
//
// Expected: VERIFICATION SUCCESSFUL (Phase 1 narrow safety baseline).

// ssif.h relies on names that ssif.cpp transitively pulls in via its own
// preamble (<algorithm>, <bit>, <span>, nv/gpio/common.h). The production
// toolchain accepts the missing forward declarations in inline factory
// bodies thanks to deferred parsing; ESBMC's frontend resolves them at
// the class-completion point and needs them visible upfront.
#include <algorithm>
#include <bit>
#include <cstdint>
#include <span>
#include "nv/gpio/common.h"
#include "nv/ssif/ssif.h"

extern "C" {
uint8_t nondet_u8();
bool    nondet_bool();
}

int main()
{
    using namespace nv::ssif;

    // Static storage gives C++-mandated zero-init of the object's bytes
    // BEFORE the user-provided ctor runs, so primitive members not in
    // the ctor's init list (_rx_cmd, _rx_size, _rx_offset, _tx_size,
    // _tx_offset) start at 0 — matching production where Ssif is a
    // static-storage member of the SSIF task.
    static nv::ipc::Event event;
    static Ssif           ssif{event};

    // bind / start are no-ops in the stubbed driver; call them so the
    // _alert_port_id / _alert_pin_id members are initialised.
    ssif.bind(nv::i2c::Port::Zero,
              static_cast<nv::gpio::GpioPort>(0),
              static_cast<nv::gpio::GpioPin>(0));
    ssif.start();

    // ---- ACK callback with nondet (address, is_read). -----------------
    // Routes to i2c_ack_bmc (consults _event.bits() — nondet under our
    // stub, exercises both txrx_pending=true / =false branches) or
    // i2c_ack_ara (calls Driver::read which returns nondet). No buffer
    // access, no copy_n, so unaffected by the bit_cast pointer-tracker
    // limitation.
    {
        uint8_t address = nondet_u8();
        bool    is_read = nondet_bool();
        (void)Ssif::i2c_ack_callback(address, is_read, &ssif);
    }

    // ---- TX handler: reads pkt.hdr.{len_msb,len_lsb} only. ------------
    // pkt.hdr is at offset 0 of the bit_cast'd Packet view; the only
    // operations are two byte reads and an integer combine. No
    // copy_n / no member-array offset, so the ESBMC pointer-tracker
    // limitation does not bite here.
    (void)ssif.handle_tx();

    // ---- RX handler: forwards _buffer to LstpRouter::send_ipmi --------
    // (no-op stub) and zeros _rx_size / _rx_offset.
    ssif.handle_rx();

    // Phase 1 (narrow): no overflow / no NaN / no OOB / no leak across
    // the entry points exercised. Replace with a Phase 3 invariant when
    // investigating a specific finding.
    __ESBMC_assert(true, "replace with Phase 3 invariant");
    return 0;
}

// Expected: VERIFICATION SUCCESSFUL (Phase 1 narrow safety baseline)
// Out of scope (deferred): i2c_callback data-path, blocked by ESBMC
// bit_cast pointer-provenance limitation on Packet::from(_buffer).
