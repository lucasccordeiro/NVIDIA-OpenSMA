// ESBMC verification stub for nv/ipc/supervisor.h
// Provides a minimal Supervisor class without FreeRTOS or IPC OS dependencies.
// Includes NV_IPC_CONFIG_H to expose GpioNum, DebugTokenEnabled, etc. used
// by nsm.h inline code.
#pragma once
#include <array>
#include <cstdint>
#include "nv/ipc/event.h"
#include "nv/ipc/mutex.h"
#include "nv/ipc/queue.h"
#include "nv/ipc/streambuffer.h"
#include "nv/ipc/task.h"
#include "nv/ipc/timer.h"
#include NV_IPC_CONFIG_H

// Provide sys::ipc::get_os_ticks() used inline in nsm.h TelemetryRecord.
namespace sys::ipc {
constexpr uint32_t get_os_ticks() { return 0; }
}  // namespace sys::ipc

namespace nv::ipc {

class Supervisor {
public:
    static Supervisor& inst() {
        static Supervisor s;
        return s;
    }
    static uint32_t get_os_ticks() { return 0; }
    static bool is_scheduler_run() { return false; }
    static bool is_in_isr()        { return false; }
};

}  // namespace nv::ipc
