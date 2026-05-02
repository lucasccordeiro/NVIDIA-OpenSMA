// Verification stub: sys/common/common.h
// Mirrors the x86 platform version (src/sys/x86/sys/common/common.h).
// Provides CoreId used by nv/ipc headers.
#pragma once
#include <cstdint>

namespace nv::ipc {

enum class CoreId : uint8_t
{
    Begin,
    Core0 = Begin,
    Core1,
    Both,
    Abstract,
    Invalid,
    End = Invalid
};

constexpr nv::ipc::CoreId get_current_core() { return nv::ipc::CoreId::Core0; }
constexpr nv::ipc::CoreId get_peer_core() { return nv::ipc::CoreId::Invalid; }

}  // namespace nv::ipc
