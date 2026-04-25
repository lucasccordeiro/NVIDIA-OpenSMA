#pragma once
#include <span>
#include "app/pdk-mctp-app-common.h"
#include "pdk-cmn-flowcontrol.h"
#include "pdk-mctp-platforms-packet.h"

namespace pdk::mctp::app {
using ::pdk::mctp::platforms::TransmitUnit;
using ::pdk::mctp::platforms::PrivHeaderSize;
using ::pdk::cmn::flowcontrol::corepdk_assert;
constexpr unsigned long PktBufDataLen = TransmitUnit + sizeof(TransportHeader);

struct [[gnu::packed]] Packet
{
    platforms::PrivateHeader                     priv;
    TransportHeader                              hdr;
    std::array<uint8_t, platforms::TransmitUnit> msg;

    std::span<uint8_t> to_span() const
    {
        return {(uint8_t*)this, sizeof(*this)};
    }

    static Packet& from(std::span<uint8_t> data)
    {
        corepdk_assert(data.size() >= sizeof(Packet),
                       "data size >= packet size assertion fail");
        return *(Packet*)data.data();
    }
};

constexpr uint32_t msgSize  = sizeof(Packet::msg);
constexpr uint32_t hdrSize  = sizeof(Packet::hdr);
constexpr uint32_t privSize = PrivHeaderSize;

}  // namespace pdk::mctp::app
