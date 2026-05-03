// ESBMC verification stub for nv/mctp/constants.h
// Provides Constants class values as literals to avoid std::max constexpr
// limitation in ESBMC's bundled <algorithm> (esbmc issue).
#pragma once
#include <cstdint>
#include "nv/mctp/interface.h"
#include NV_IPC_CONFIG_H

namespace nv::mctp {

constexpr uint32_t NsmV2ChunkHeaderSize = 12;

class Constants {
public:
    constexpr static uint32_t PldmRxBufSize  = 256;
    constexpr static uint32_t SpdmRxBufSize  = 256;
    constexpr static uint32_t MctpTxBufSize  = 256;

    constexpr static uint8_t  BufferSize     = PktBufDataLen + sizeof(mctp::PrivateHeader);
    constexpr static uint32_t MultiPktBufSize = 32 + 4096;
    constexpr static uint8_t  HeaderSize     = 5;
    constexpr static uint32_t UsbBufSize     = 512;

    // NSM response header sizes.
    constexpr static uint8_t NsmHeaderResponseSize        = 12;
    constexpr static uint8_t NsmHeaderReasonResponseSize  = 10;
    constexpr static uint8_t NsmAggregateHeaderResponseSize = 10;
    constexpr static uint8_t NsmHeaderRequestSize         = 8;
    constexpr static uint8_t NsmHeaderRequestSizeV2       = 12;
    constexpr static uint8_t NsmHeaderEventMsgSize        = 12;
    constexpr static uint8_t NsmType4ResponseSize         = 10;
};

}  // namespace nv::mctp
