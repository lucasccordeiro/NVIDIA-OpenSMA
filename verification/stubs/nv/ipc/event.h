// ESBMC verification stub for nv/ipc/event.h
#pragma once
#include <chrono>
#include <cstdint>
#include <limits>
#include "nv/common/expected.h"
#include "nv/ipc/object.h"
#include NV_IPC_CONFIG_H

extern "C" uint32_t nondet_u32();

namespace nv::ipc {

class Event : public Object {
public:
    using IdType = EventId;
    using Bits   = uint32_t;
    using Usecs  = std::chrono::microseconds;

    enum class Status { Ok, Timeout, InvalidParam, Unknown, InvalidOperation, CommunicationFailed, InsufficientPermissions };

    using Return = nv::common::Expected<Bits, Status>;

    Event() noexcept = default;

    static Event& make(EventId) {
        static Event e;
        return e;
    }
    EventId id() const { return _id; }
    // bits() returns a nondet bitmap so the harness explores both
    // `txrx_pending == true` and `txrx_pending == false` branches in
    // Ssif::i2c_ack_bmc / smbus_block_write / smbus_block_read.
    Return  bits() const { return Return{nondet_u32()}; }
    Status  set(Bits, CoreId = CoreId::Invalid) { return Status::Ok; }
    Status  clear(Bits = std::numeric_limits<Bits>::max()) { return Status::Ok; }
    Return  wait(Bits, bool = true, bool = false, Usecs = Usecs::max())
    {
        return Return{nondet_u32()};
    }

private:
    EventId _id = EventId::End;
};

}  // namespace nv::ipc
