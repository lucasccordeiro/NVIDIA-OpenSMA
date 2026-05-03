// ESBMC verification stub for nv/mctp/nsm_type_4.h
// Identical to the production header except that DsInterfaceErrorTable is
// declared `inline const` instead of `constexpr inline`: ESBMC's bundled
// <tuple> does not mark std::tuple as a literal type, so a constexpr
// std::array<std::tuple<...>, N> fails to compile (esbmc issue).
#pragma once
#include <array>
#include <climits>
#include <cstddef>
#include <cstdint>

#include "nv/i2c/lattice_driver.h"
#include "nv/mctp/constants.h"
#include "nv/perf_mon/perf_mon.h"

namespace nv::mctp {

constexpr uint8_t DevDiagGetDiagnosticsNoMoreSegments = 0xFF;
constexpr uint8_t DevDiagTimestampTagId               = 0XFF;
constexpr uint8_t DevDiagGetDiagnosticsFirstSegment   = 0x00;

constexpr uint8_t DevDiagGetResetStatisticsResetCauseId = 0x08;
constexpr uint8_t GetResetStatisticsResetLen            = 0x3;

constexpr uint8_t  CpldRegisterTableNoMoreSegments = 0xFF;
constexpr uint8_t  CpldRegisterTableFirstSegment   = 0x00;
constexpr uint16_t CpldRegTablePayloadMaxSize =
    Constants::MctpTxBufSize - sizeof(nv::mctp::PrivateHeader)
    - sizeof(nv::mctp::Header) - Constants::NsmHeaderResponseSize;

constexpr uint8_t  NoMoreResetTargets    = 0xFF;
constexpr uint16_t ResetInProgress       = 0x0000;
constexpr uint16_t TimeCounterSaturated  = 0xFFFF;
constexpr uint16_t ResetTime1Ms          = 0x0001;
constexpr uint8_t  ReservedField         = 0x00;

enum class RecoveryLevel : uint8_t {
    ApplicationReset = 0,
    ProtocolReset    = 1,
    PortReset        = 2,
    OobHardwareReset = 3,
    QueryNextTarget  = 255,
};

enum class AggregateTaskEvent : uint32_t {
    Mctp   = nv::common::bit(0),
    I2C    = nv::common::bit(1),
    I3C    = nv::common::bit(2),
    PLDM   = nv::common::bit(3),
    USB    = nv::common::bit(4),
    Flash  = nv::common::bit(5),
    Logger = nv::common::bit(6),
    SPDM   = nv::common::bit(7),
};

struct [[gnu::packed]] TaskExecutionTimeResp {
    uint8_t                         task_id;
    nv::perf_mon::TaskExecutionTime execution_time;
};

struct [[gnu::packed]] T4FlashUsageResp {
    uint32_t usage;
    uint32_t total_slots;
};

struct [[gnu::packed]] T4RamSizeResp {
    uint32_t usage;
    uint32_t total;
};

using T4TaskIdAndPriorityResponse =
    std::array<std::pair<uint8_t, uint8_t>, static_cast<uint8_t>(nv::ipc::TaskId::KernelEnd)>;

struct [[gnu::packed]] T4ErrorCounterResponse {
    uint8_t                        latached_error;
    nv::perf_mon::OobBusErrorCount error_count;
};

using DsMcpInterface   = pdk::mctp::platforms::Interface;
using DsOobBusError    = nv::perf_mon::OobBus;
using DsInterfaceError = std::tuple<DsMcpInterface, DsOobBusError>;
constexpr auto T4DsInterfaceErrorNum = 13;

// `inline const` instead of `constexpr inline`: ESBMC bundled <tuple> does
// not treat std::tuple as a literal type (esbmc issue).
inline const std::array<DsInterfaceError, T4DsInterfaceErrorNum> DsInterfaceErrorTable = {
    DsInterfaceError{pdk::mctp::platforms::Interface::DsI2c0, nv::perf_mon::OobBus::DsI2c0},
    DsInterfaceError{pdk::mctp::platforms::Interface::DsI2c1, nv::perf_mon::OobBus::DsI2c1},
    DsInterfaceError{pdk::mctp::platforms::Interface::DsI2c2, nv::perf_mon::OobBus::DsI2c2},
    DsInterfaceError{pdk::mctp::platforms::Interface::DsI2c3, nv::perf_mon::OobBus::DsI2c3},
    DsInterfaceError{pdk::mctp::platforms::Interface::DsI2c4, nv::perf_mon::OobBus::DsI2c4},
    DsInterfaceError{pdk::mctp::platforms::Interface::DsI2c5, nv::perf_mon::OobBus::DsI2c5},
    DsInterfaceError{pdk::mctp::platforms::Interface::DsI2c6, nv::perf_mon::OobBus::DsI2c6},
    DsInterfaceError{pdk::mctp::platforms::Interface::DsI2c7, nv::perf_mon::OobBus::DsI2c7},
    DsInterfaceError{pdk::mctp::platforms::Interface::DsI3c0, nv::perf_mon::OobBus::DsI3c0},
    DsInterfaceError{pdk::mctp::platforms::Interface::DsI3c1, nv::perf_mon::OobBus::DsI3c1},
    DsInterfaceError{  pdk::mctp::platforms::Interface::Spi0, nv::perf_mon::OobBus::Spi0  },
    DsInterfaceError{  pdk::mctp::platforms::Interface::Spi1, nv::perf_mon::OobBus::Spi1  },
    DsInterfaceError{  pdk::mctp::platforms::Interface::Spi2, nv::perf_mon::OobBus::Spi2  },
};

struct [[gnu::packed]] BridgePortRecoveryReq {
    uint8_t recovery_level;
    uint8_t reset_target;
};

struct [[gnu::packed]] BridgePortRecoveryResp {
    uint8_t  next_reset_target;
    uint8_t  reserved;
    uint16_t time_since_last_reset;
};

enum T4WriteProtectionMode : uint8_t { Clear = 0, Set = 1 };

struct [[gnu::packed]] T4WriteProtectionRequest {
    uint8_t function;
    uint8_t mode;
    T4WriteProtectionRequest() : function{0}, mode{0} {}
};

NsmStatus platform_write_protection_gpio(uint8_t function, uint8_t mode);

struct [[gnu::packed]] CpldRegisterTableReq {
    uint8_t segment_index;
};

struct [[gnu::packed]] CpldRegisterTableResp {
    uint8_t next_segment;
    uint8_t segment_data[Cpld_User_Reg::CPLD_USER_REG_SIZE];
};

}  // namespace nv::mctp
