// Verification-only overlay of corepdk/modules/mctp-cpp/src/app/pdk-mctp-app-packet.h
//
// After esbmc#4184 (which fixed esbmc#4180 part 2 — namespace-qualified
// constexpr) the only remaining delta from production is the cast form
// inside Packet::to_span / Packet::from. Using std::bit_cast<T*> here
// causes:
//   (a) esbmc#4180 part 1 — converter crash on bit_cast inside class
//       methods (still open as of 2026-04-26), and
//   (b) a separate Phase-2 unsoundness: even when bit_cast succeeds, ESBMC
//       reports a spurious counterexample on the round-trip
//       `get_packet_length(*bit_cast<Packet*>(span.data())) == set_value`,
//       despite the CEX itself showing `mirror = &pkt`. Likely an aliasing-
//       through-bit_cast resolution bug in ESBMC; to be reported.
//
// The reinterpret_cast / C-cast form below is semantically equivalent for
// the trivially-copyable [[gnu::packed]] Packet layout. Tag tracked as
// WORKAROUND esbmc#4180 (part 1).
#pragma once
#include <span>

#include "app/pdk-mctp-app-common.h"
#include "pdk-cmn-flowcontrol.h"
#include "pdk-mctp-platforms-packet.h"

namespace pdk::mctp::app {

constexpr auto PktBufDataLen = platforms::TransmitUnit + sizeof(TransportHeader);

struct [[gnu::packed]] Packet
{
    platforms::PrivateHeader                     priv;
    TransportHeader                              hdr;
    std::array<uint8_t, platforms::TransmitUnit> msg;

    // WORKAROUND esbmc#4180 part 1: reinterpret_cast in lieu of std::bit_cast.
    std::span<uint8_t> to_span() const
    {
        return {(uint8_t*)this, sizeof(*this)};
    }

    // WORKAROUND esbmc#4180 part 1: reinterpret_cast in lieu of std::bit_cast.
    static Packet& from(std::span<uint8_t> data)
    {
        pdk::cmn::flowcontrol::corepdk_assert(
            data.size() >= sizeof(Packet),
            "data size >= packet size assertion fail");
        return *(Packet*)data.data();
    }
};

constexpr uint32_t msgSize  = sizeof(Packet::msg);
constexpr uint32_t hdrSize  = sizeof(Packet::hdr);
constexpr uint32_t privSize = platforms::PrivHeaderSize;

}  // namespace pdk::mctp::app
