// ESBMC verification stub for sys/flash/flash_config.h
#pragma once
#include <array>
#include <cstdint>
#include <limits>
#include <utility>

namespace sys::flash::config {

using Address                 = uint32_t;
constexpr uint32_t SectorSize = 0x2000;
constexpr uint32_t PhraseSize = 16;

constexpr Address RemapSize   = 0x80000;
constexpr Address MaxAddress  = 0x100000;

constexpr Address FmcFwAddress = 0x1008000;
constexpr Address FmcFwSize    = 0x8000;

constexpr Address LoggerStartAddress = 0x70000;
constexpr Address PdsAddressStart    = 0xF0000;
constexpr Address DebugTokenSpiOffset = 0xEE000;

constexpr Address Slot0FwAddress  = 0x0;
constexpr Address Slot1FwAddress  = RemapSize;
constexpr Address FirmwareMaxSize = 0x60000;

constexpr uint16_t McuComponentId = 0xff02;

}  // namespace sys::flash::config
