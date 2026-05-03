// ESBMC verification stub for nv/mctp/nsm_pwr_smoothing_handlers.h
// All power-smoothing handlers are no-ops; they are compiled but never called
// by the F-6 harness (process_device_configuration / SetErrorInjectionMode).
#pragma once
#include <cstdint>

namespace nv::mctp {
struct NsmPktResp;
}

namespace pdk::mctp::platforms {
enum class Ccode : uint8_t;
}

namespace nv::mctp {
using Ccode = pdk::mctp::platforms::Ccode;
}

namespace nv::mctp::nsm_pwr_smoothing_handlers {

inline Ccode handle_get_max_ac_ramp_rate([[maybe_unused]] nv::mctp::NsmPktResp&) {
    return static_cast<Ccode>(0);
}
inline Ccode handle_get_soc_power_smooth_enabled([[maybe_unused]] nv::mctp::NsmPktResp&) {
    return static_cast<Ccode>(0);
}
inline Ccode handle_get_soc_power_smooth_current_preset([[maybe_unused]] nv::mctp::NsmPktResp&) {
    return static_cast<Ccode>(0);
}
inline Ccode handle_get_soc_power_brake_enabled([[maybe_unused]] nv::mctp::NsmPktResp&) {
    return static_cast<Ccode>(0);
}
inline Ccode handle_get_soc_therm_brake_enabled([[maybe_unused]] nv::mctp::NsmPktResp&) {
    return static_cast<Ccode>(0);
}
inline Ccode handle_get_rack_power_smoothing_param([[maybe_unused]] nv::mctp::NsmPktResp&) {
    return static_cast<Ccode>(0);
}
inline Ccode handle_get_rack_power_smoothing_testhook([[maybe_unused]] nv::mctp::NsmPktResp&) {
    return static_cast<Ccode>(0);
}
inline Ccode handle_get_debug_telemetry([[maybe_unused]] uint16_t, [[maybe_unused]] nv::mctp::NsmPktResp&) {
    return static_cast<Ccode>(0);
}
inline Ccode handle_get_adc_calibration_results([[maybe_unused]] nv::mctp::NsmPktResp&) {
    return static_cast<Ccode>(0);
}
inline Ccode handle_get_power_smooth_raw_readback([[maybe_unused]] uint8_t, [[maybe_unused]] nv::mctp::NsmPktResp&) {
    return static_cast<Ccode>(0);
}
inline Ccode handle_set_max_ac_ramp_rate([[maybe_unused]] float) {
    return static_cast<Ccode>(0);
}
inline Ccode handle_set_soc_power_smooth_enabled([[maybe_unused]] bool) {
    return static_cast<Ccode>(0);
}
inline Ccode handle_set_soc_power_smooth_current_preset([[maybe_unused]] uint8_t) {
    return static_cast<Ccode>(0);
}
inline Ccode handle_set_soc_power_brake_enabled([[maybe_unused]] bool) {
    return static_cast<Ccode>(0);
}
inline Ccode handle_set_soc_therm_brake_enabled([[maybe_unused]] bool) {
    return static_cast<Ccode>(0);
}
inline Ccode handle_set_rack_power_smoothing_param(
    [[maybe_unused]] uint8_t, [[maybe_unused]] uint32_t, [[maybe_unused]] uint32_t,
    [[maybe_unused]] uint32_t, [[maybe_unused]] uint8_t) {
    return static_cast<Ccode>(0);
}
inline Ccode handle_set_rack_power_smoothing_testhook(
    [[maybe_unused]] uint8_t, [[maybe_unused]] uint32_t, [[maybe_unused]] uint32_t,
    [[maybe_unused]] uint32_t, [[maybe_unused]] uint8_t) {
    return static_cast<Ccode>(0);
}
inline Ccode handle_trigger_adc_calibration() { return static_cast<Ccode>(0); }
inline Ccode handle_adc_calib_set_loopback_dac_code([[maybe_unused]] uint16_t) {
    return static_cast<Ccode>(0);
}

}  // namespace nv::mctp::nsm_pwr_smoothing_handlers
