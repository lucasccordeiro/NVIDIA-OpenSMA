// ESBMC verification stub for nv/flash/common.h
#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include "nv/flash/datastore.h"
#include "sys/flash/flash_config.h"

namespace nv::flash {

using Address = sys::flash::config::Address;

constexpr uint32_t SectorSize  = sys::flash::config::SectorSize;
constexpr uint32_t PhraseSize  = sys::flash::config::PhraseSize;
constexpr Address  RemapSize   = sys::flash::config::RemapSize;
constexpr Address  MaxAddress  = sys::flash::config::MaxAddress;
constexpr Address  FmcFwAddress = sys::flash::config::FmcFwAddress;
constexpr Address  FmcFwSize   = sys::flash::config::FmcFwSize;

constexpr uint32_t BufferSize = 256;
using Buffer = std::array<uint8_t, BufferSize>;

enum class Status : uint32_t {
    Ok = 0,
    Error,
    Busy,
    Timeout,
    InvalidParam,
};

}  // namespace nv::flash
