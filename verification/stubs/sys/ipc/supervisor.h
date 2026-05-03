// ESBMC verification stub for sys/ipc/supervisor.h
#pragma once
#include <cstdint>
#include NV_IPC_CONFIG_H

namespace sys::ipc {

struct Supervisor {};

constexpr uint32_t calc_static_c2c_buf_size() noexcept { return 0; }
constexpr uint32_t get_os_ticks() { return 0; }

}  // namespace sys::ipc
