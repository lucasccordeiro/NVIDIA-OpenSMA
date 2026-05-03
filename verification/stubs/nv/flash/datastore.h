// ESBMC verification stub for nv/flash/datastore.h
#pragma once
#include <cstdint>

namespace nv::flash {

using Data = uint32_t;

enum class Key : uint32_t {
    NpdsStart             = 0x0,
    NpdsBootReason        = NpdsStart,
    NpdsEnd,
    NpdsInvalid,
    PdsStart              = 0x1000,
    PdsNcsiMacAddrLow     = PdsStart,
    PdsNcsiMacAddrHigh,
    PdsEnd,
    Invalid               = 0xFFFFFFFF,
};

}  // namespace nv::flash
