// NV_IPC_CONFIG_H substitute for NSM F-6 system-level ESBMC harness.
// Provides all hardware-platform constants and IPC type stubs needed to
// compile nv/mctp/nsm.h and nv/mctp/nsm_type_5.cpp without FreeRTOS.
#pragma once
#include <array>
#include <cstdint>
#include <tuple>
#include "nv/common/utils.h"

// CoreId is provided by the existing sys/common/common.h stub.
// Include it here so NV_IPC_CONFIG_H is self-contained.
#include "sys/common/common.h"

namespace nv::ipc {

// GPIO: 6 ports × 32 pins = 192 pins total.
constexpr uint16_t GpioNum = 192;

// Feature flags — all disabled for verification.
constexpr bool DebugTokenEnabled = false;
constexpr bool EnableDualCore    = false;
constexpr bool EnableLstp        = false;

// Voltage monitor: no leak-detect sensors in verification.
namespace voltage_monitor_config {
constexpr uint32_t LeakDetectSensorNum = 0;
}  // namespace voltage_monitor_config

// IPC object ID enumerations — minimal sets sufficient for verification.
enum class TaskId : uint8_t {
    Begin    = 0,
    Mctp     = Begin,
    KernelEnd,
    End = KernelEnd,
};

enum class EventId : uint8_t { End = 0 };
enum class QueueId : uint8_t { End = 0 };
enum class MutexId : uint8_t { End = 0 };
enum class TimerId : uint8_t { End = 0 };

// Queue configuration: no queues in verification.
// Use inline (not constexpr): ESBMC bundled <array> does not treat
// std::array<tuple<...>, 0> as a constexpr-capable type.
using QueueInfo = std::tuple<QueueId, std::size_t, std::size_t>;
inline const std::array<QueueInfo, 0> QueueInfos{};

// MCTP driver constants.
constexpr uint32_t EndpointStatusChangePeriodMs = 1000;
constexpr uint8_t  DownStreamNum = 1;

// SPDM request queue size (used in nv/mctp/constants.h).
constexpr uint32_t SpdmRequestQueueSize = 256;

}  // namespace nv::ipc

namespace nv::lstp {
constexpr bool EnableI2c = false;
}  // namespace nv::lstp

// I2C error-injection constants at global scope (used by production
// error_injection.h's MaxErrorInjectionPorts = NV_I2C_MAX_ERROR_INJECTION_PORTS).
constexpr uint32_t NV_I2C_MAX_ERROR_INJECTION_PORTS = 0;

namespace nv::i2c {
// These must also live in nv::i2c so that nsm_type_5.cpp can reference them
// as nv::i2c::NV_I2C_ERROR_INJECTION_PORTS etc.
constexpr size_t NV_I2C_ERROR_INJECTION_PORTS = 0;
constexpr size_t NV_IOX_ERROR_INJECTION_PORTS = 0;
constexpr size_t NV_I2C_MAX_ERROR_INJECTION_PORTS =
    NV_I2C_ERROR_INJECTION_PORTS + NV_IOX_ERROR_INJECTION_PORTS;
}  // namespace nv::i2c

// NSM bitmask constants (needed for config_nsm_* signatures below).
#include "nv/mctp/nsm_msg_bitmask.h"

namespace nv::mctp {

// Sensor sizes: zero for verification — the nsm.h if-constexpr branches that
// add telemetry command codes are dead-code-eliminated when sizes are zero.
constexpr uint32_t mcuPowerSensorsSize   = 0;
constexpr uint32_t mcuVoltageSensorsSize = 0;
constexpr uint8_t  I2cTempSensorSize     = 0;

// Platform hooks that add product-specific NSM command codes to the
// bitmasks built by gen_*_bitmask() in nsm.h.  All are no-ops here.
constexpr void config_nsm_types(
    [[maybe_unused]] std::array<uint8_t, nsm_msg::NvMctpSupportedNum>& b) {}
constexpr void config_nsm_type0_cmd(
    [[maybe_unused]] std::array<uint8_t, nsm_msg::NvMctpSupportedNum>& b) {}
constexpr void config_nsm_type2_cmd(
    [[maybe_unused]] std::array<uint8_t, nsm_msg::NvMctpSupportedNum>& b) {}
constexpr void config_nsm_type3_cmd(
    [[maybe_unused]] std::array<uint8_t, nsm_msg::NvMctpSupportedNum>& b) {}
constexpr void config_nsm_type4_cmd(
    [[maybe_unused]] std::array<uint8_t, nsm_msg::NvMctpSupportedNum>& b) {}
constexpr void config_nsm_type5_cmd(
    [[maybe_unused]] std::array<uint8_t, nsm_msg::NvMctpSupportedNum>& b) {}
constexpr void config_nsm_type6_cmd(
    [[maybe_unused]] std::array<uint8_t, nsm_msg::NvMctpSupportedNum>& b) {}
constexpr void config_nsm_typeff_cmd(
    [[maybe_unused]] std::array<uint8_t, nsm_msg::NvMctpSupportedNum>& b) {}

// Platform hook: returns empty event bitmask; type0 events not configured here.
constexpr std::array<uint8_t, nsm_msg::NvMctpEventSupportedNum> gen_type0_event_bitmask()
{
    return {};
}

}  // namespace nv::mctp

// GpioSetup and GpioNsmEventMask are platform-specific arrays that live in
// nv::ipc and are referenced by nsm_type_5.cpp.  They require nv::gpio types,
// so include those stubs first.
#include "nv/gpio/common.h"
#include "sys/gpio/constant.h"

namespace nv::ipc {

// GPIO setup: 192 (GpioNum) entries, all default (InvalidGpioPort/Pin) as
// nsm_type_5.cpp only checks .size() and guards with bounds checks before use.
using Gpios = std::tuple<nv::gpio::GpioPort, nv::gpio::GpioPin>;
constexpr inline std::array<Gpios, GpioNum> GpioSetup{};

// GPIO NSM event masks: all zero — no GPIO events configured in verification.
// Size must equal sys::gpio::PortsNumber + 1 = 7 (checked by static_assert in
// nsm_type_5.cpp).
inline constexpr std::array<uint32_t, sys::gpio::PortsNumber + 1> GpioNsmEventMask{};
inline constexpr std::array<uint32_t, sys::gpio::PortsNumber + 1> GpioNsmEventAssertMask{};

}  // namespace nv::ipc
