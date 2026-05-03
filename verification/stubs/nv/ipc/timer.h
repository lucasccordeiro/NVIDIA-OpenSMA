// ESBMC verification stub for nv/ipc/timer.h
#pragma once
#include <chrono>
#include "nv/ipc/object.h"
#include NV_IPC_CONFIG_H

namespace nv::ipc {

class Timer : public Object {
public:
    using IdType = TimerId;
    using Usecs  = std::chrono::microseconds;

    Timer() noexcept = default;

    static Timer& make(TimerId) {
        static Timer t;
        return t;
    }
    TimerId id() const { return _id; }
    void start(Usecs = Usecs{0x7FFFFFFFFFFFFFFF}) {}
    void stop() {}

private:
    const TimerId _id = TimerId::End;
};

}  // namespace nv::ipc
