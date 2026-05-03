// ESBMC verification stub for nv/mctp/task.h
#pragma once
#include "nv/i2c/powersensor/device_manager.h"

namespace nv::mctp {

class Task {
public:
    nv::i2c::power::DeviceManager& power_sensor_mgr() { return power_sensor_mgr_; }
    void set_port_recovery_ei_bitmap([[maybe_unused]] uint32_t bitmap) {}
private:
    nv::i2c::power::DeviceManager power_sensor_mgr_;
};

inline nv::i2c::power::DeviceManager& get_power_sensor_mgr() {
    static nv::i2c::power::DeviceManager mgr;
    return mgr;
}

inline Task& get_task() {
    static Task t;
    return t;
}

}  // namespace nv::mctp
