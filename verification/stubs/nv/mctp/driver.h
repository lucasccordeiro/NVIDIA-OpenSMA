// ESBMC verification stub for nv/mctp/driver.h
// Provides Driver::CmdCode and mctp_send_cmd() as a no-op.
#pragma once
#include <cstdint>

namespace nv::mctp {

enum class DriverStatus : uint8_t { Ok, QueueSendFail, EventSetFail, InvalidClient, Unknown };

class Driver {
public:
    enum class CmdCode : uint16_t {
        NsmT5FatalFaultEI = 0,
    };

    static DriverStatus mctp_send_cmd(CmdCode, uint8_t) { return DriverStatus::Ok; }
};

}  // namespace nv::mctp
