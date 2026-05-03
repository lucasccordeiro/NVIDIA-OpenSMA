// ESBMC verification stub for nv/ipc/streambuffer.h
#pragma once
#include <cstdint>
#include "sys/ipc/streambuffer.h"
#include NV_IPC_CONFIG_H

namespace nv::ipc {

class StreamBuffer {
public:
    using IdType = uint32_t;
    StreamBuffer() noexcept = default;
};

}  // namespace nv::ipc
