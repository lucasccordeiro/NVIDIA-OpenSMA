// ESBMC verification stub for nv/ipc/task.h
// Replaces the FreeRTOS-dependent production header.
#pragma once
#include <cstdint>
#include <string_view>
#include "sys/ipc/task.h"
#include NV_IPC_CONFIG_H

namespace nv::ipc {

class Task : public sys::ipc::Task {
public:
    Task() = delete;

    auto id()   const { return _id; }
    auto name() const { return _name; }

    using PairIdAndPriority  = std::pair<TaskId, int>;
    using TaskIdAndPriority  = std::array<PairIdAndPriority, static_cast<uint8_t>(TaskId::KernelEnd)>;
    static void get_task_id_and_priority(TaskIdAndPriority&) {}

protected:
    Task(TaskId id, std::string_view name) noexcept : _id(id), _name(name) {}

private:
    TaskId           _id;
    std::string_view _name;
};

class TaskPublic : public sys::ipc::TaskPublic {
public:
    auto& id() { return _id; }
    Task* task{};
private:
    TaskId _id{};
};

}  // namespace nv::ipc
