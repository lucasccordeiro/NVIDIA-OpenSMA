// ESBMC verification stub for nv/ipc/mutex.h
#pragma once
#include <chrono>
#include "nv/ipc/object.h"
#include NV_IPC_CONFIG_H

namespace nv::ipc {

class Mutex : public Object {
public:
    using IdType = MutexId;
    using Usecs  = std::chrono::microseconds;
    static constexpr Usecs DefaultTimeout = Usecs(1'000'000);

    enum class Status { Ok, Timeout, Error };

    Mutex() noexcept = default;

    static Mutex& make(MutexId) {
        static Mutex m;
        return m;
    }
    MutexId id() const { return _id; }
    [[nodiscard]] Status lock(Usecs = DefaultTimeout) { return Status::Ok; }
    void unlock() {}

private:
    const MutexId _id = MutexId::End;
};

}  // namespace nv::ipc
