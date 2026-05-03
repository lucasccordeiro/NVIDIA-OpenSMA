// ESBMC verification stub for nv/ipc/queue.h
#pragma once
#include <chrono>
#include <cstdint>
#include <span>
#include "nv/ipc/object.h"
#include NV_IPC_CONFIG_H

namespace nv::ipc {

class Queue : public Object {
public:
    using IdType    = QueueId;
    using Item      = std::span<uint8_t>;
    using ConstItem = std::span<const uint8_t>;
    using Usecs     = std::chrono::microseconds;

    enum class Status { Ok, Timeout, InvalidParam, Empty, Full, Unknown, InvalidOperation, CommunicationFailed, InsufficientPermissions };

    Queue() noexcept = default;

    static Queue& make(QueueId) {
        static Queue q;
        return q;
    }

    QueueId     id()        const { return QueueId::End; }
    std::size_t item_size() const { return 0; }
    std::size_t max_items() const { return 0; }
    std::size_t size()      const { return 0; }
    std::size_t available() const { return 0; }

    [[nodiscard]] Status send(const ConstItem&, Usecs = Usecs{0x7FFFFFFFFFFFFFFF}, CoreId = CoreId::Invalid) { return Status::Ok; }
    [[nodiscard]] Status send_isr(const ConstItem&) { return Status::Ok; }
    [[nodiscard]] Status recv(Item&, Usecs = Usecs{0x7FFFFFFFFFFFFFFF}) { return Status::Empty; }
    [[nodiscard]] Status recv_isr(Item&) { return Status::Empty; }
    [[nodiscard]] Status send_front(const ConstItem&, Usecs = Usecs{0x7FFFFFFFFFFFFFFF}) { return Status::Ok; }
    [[nodiscard]] Status send_front_isr(const ConstItem&) { return Status::Ok; }
    void reset() {}
};

constexpr std::size_t get_queue_max_items(QueueId) { return 0; }

}  // namespace nv::ipc
