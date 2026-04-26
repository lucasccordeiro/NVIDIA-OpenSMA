// Verification-only overlay of corepdk/modules/mctp-cpp/src/app/pdk-mctp-app-packet.h
//
// Only delta from production is the cast form inside Packet::to_span /
// Packet::from. Using `std::bit_cast<T*>(...)` parses cleanly post-esbmc#4184
// but produces a spurious round-trip CEX whose own trace shows
// `mirror = &pkt` — see esbmc#4191. The C-cast form below is semantically
// identical for the trivially-copyable [[gnu::packed]] Packet layout; remove
// once esbmc#4191 is fixed.
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

    // WORKAROUND esbmc#4191: C-cast in lieu of std::bit_cast (aliasing CEX).
    std::span<uint8_t> to_span() const
    {
        return {(uint8_t*)this, sizeof(*this)};
    }

    // WORKAROUND esbmc#4191: C-cast in lieu of std::bit_cast (aliasing CEX).
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
