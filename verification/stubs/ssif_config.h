// NV_IPC_CONFIG_H substitute for nv::ssif::Ssif ESBMC harness.
//
// Provides only the IPC-config surface ssif.{h,cpp} actually reaches:
//   - nv::ipc::CoreId, EventId, UsbLstpMsgSize
//   - nv::lstp::EnableLstp = false (gates lstp_router.h template paths)
//   - NUM_I2C_TARGET_ADDRESSES = 2 (BMC + ARA, matches production ssif)
//
// Same shape as nsm_f6_config.h; intentionally minimal to keep the
// verification surface bounded.
#pragma once
#include <array>
#include <cstdint>

#include "sys/common/common.h"  // provides nv::ipc::CoreId

namespace nv::ipc {

enum class EventId : uint8_t
{
    Begin = 0,
    Ssif  = Begin,
    End,
};

// LSTP USB message size — production p3957_cxx / testrunner both use 512.
constexpr uint32_t UsbLstpMsgSize = 512;

}  // namespace nv::ipc

namespace nv::lstp {
constexpr bool EnableLstp = false;
constexpr bool EnableGpio = false;
constexpr bool EnableI2c  = false;
}  // namespace nv::lstp

// I2C slave target slots: BMC (0x10) + ARA (0x0c) = 2.
#ifndef NUM_I2C_TARGET_ADDRESSES
#define NUM_I2C_TARGET_ADDRESSES 2
#endif
