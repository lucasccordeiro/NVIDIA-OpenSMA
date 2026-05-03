// ESBMC verification stub for nv/ipc/event.h
#pragma once
#include <chrono>
#include <cstdint>
#include <limits>
#include "nv/ipc/object.h"
#include NV_IPC_CONFIG_H

namespace nv::ipc {

class Event : public Object {
public:
    using IdType = EventId;
    using Bits   = uint32_t;
    using Usecs  = std::chrono::microseconds;

    enum class Status { Ok, Timeout, InvalidParam, Unknown, InvalidOperation, CommunicationFailed, InsufficientPermissions };

    Event() noexcept = default;

    static Event& make(EventId) {
        static Event e;
        return e;
    }
    EventId id() const { return _id; }
    Status  set(Bits, CoreId = CoreId::Invalid) { return Status::Ok; }
    Status  clear(Bits = std::numeric_limits<Bits>::max()) { return Status::Ok; }

private:
    EventId _id = EventId::End;
};

}  // namespace nv::ipc
