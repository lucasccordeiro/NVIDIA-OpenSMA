// ESBMC verification stub for nv/perf_mon/perf_mon.h
#pragma once
#include <array>
#include <cstdint>

namespace nv::perf_mon {

constexpr uint32_t CpuUtilizationEntryNum    = 20;
constexpr uint32_t TaskExecutionTimeEntryNum = 20;
constexpr uint32_t TaskNum                   = 1;

using CpuUtilization       = std::array<uint32_t, CpuUtilizationEntryNum>;
using TaskExecutionTime    = std::array<uint32_t, TaskExecutionTimeEntryNum>;
using AllTaskExecutionTime = std::array<TaskExecutionTime, TaskNum>;

enum class OobBus : uint8_t {
    Begin = 0,
    UsI2c = Begin,
    DsI2c0, DsI2c1, DsI2c2, DsI2c3,
    DsI3c0, DsI3c1,
    Spi0, Spi1, Spi2,
    DsI2c4, DsI2c5, DsI2c6, DsI2c7,
    End
};

constexpr uint8_t  error_type_num  = 16;
constexpr uint32_t OobBusTypeNum   = static_cast<uint32_t>(OobBus::End);
using OobBusErrorCount  = std::array<uint32_t, error_type_num>;
using OobBusError       = std::array<OobBusErrorCount, OobBusTypeNum>;
using OobBusErrorLatched = std::array<uint8_t, OobBusTypeNum>;
constexpr uint32_t OobBusErrorEntryNum = 20;
using OobBusErrorBuf    = std::array<OobBusError, OobBusErrorEntryNum>;
using OobBusErrorRecent = std::array<OobBusErrorCount, OobBusTypeNum>;

inline uint32_t get_tick() { return 0; }

}  // namespace nv::perf_mon
