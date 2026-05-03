// ESBMC verification stub for nv/telemetry/cache.h
#pragma once
#include <array>
#include <cstdint>

namespace nv::telemetry {

struct Cache {
    using Value = uint32_t;
    static constexpr uint32_t InvalidItem = 0xFFFFFFFF;
};

}  // namespace nv::telemetry
