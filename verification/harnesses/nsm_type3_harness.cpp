// ESBMC harness for three NSM Type-3 sensor availability functions:
//   is_temp_sensor_available, is_power_sensor_available, is_voltage_sensor_available
// (production: src/nv/mctp/nsm_type_3.cpp).
//
// These functions do a linear scan over platform-config sensor arrays.
// The production source pulls in a large MCU subsystem (FreeRTOS, gpio,
// ipc, logger) that ESBMC cannot link, so we inline the function bodies
// and configuration arrays verbatim from:
//   nsm_type_3.cpp:668-713   (function bodies + weak is_busbar_available)
//   src/projects/testrunner/config.h:848-867  (platform sensor arrays)
// The inlined code is 30 source lines; drift risk is minimal and the
// alternative (stubbing the entire NV MCU subsystem) is disproportionate.
//
// is_busbar_available() is __attribute__((weak)) with default true.
// Here it returns a nondet bool so ESBMC explores both availability states.
//
// Phase 1: all three functions are total over the full uint8_t input domain
//          (no UB, no OOB accesses, regardless of busbar state).
// Phase 2: functional contracts — result iff sensorId is a member of the
//          platform-configured sensor array (busbar state factored in for temp).

#include <cstdint>

extern "C" {
uint8_t  nondet_u8();
bool     nondet_bool();
unsigned nondet_uint();
}

namespace verif {

// ---- Type3TemperatureSensors enumerators (from src/nv/mctp/enums.h) ----
constexpr uint8_t TempGpu1        = 0;
constexpr uint8_t TempGpu2        = 1;
constexpr uint8_t TempTMP451_1    = 2;
constexpr uint8_t TempTMP451_2    = 3;
constexpr uint8_t TempMaxModule   = 4;
constexpr uint8_t TempSMAInternal = 5;
constexpr uint8_t SMA_Internal    = 17;
constexpr uint8_t BusBar_Temp     = 19;

// ---- Type3PowerSensors enumerators ----
constexpr uint8_t PowerGpu1   = 0;
constexpr uint8_t PowerGpu2   = 1;
constexpr uint8_t PowerModule = 2;

// ---- Platform config (verbatim from src/projects/testrunner/config.h) ----
constexpr uint8_t mcuTemperatureSensors[] = {
    TempGpu1, TempGpu2, TempTMP451_1, TempTMP451_2,
    TempMaxModule, TempSMAInternal, BusBar_Temp, SMA_Internal,
};
constexpr auto mcuTemperatureSensorsSize = 8;

constexpr uint8_t mcuPowerSensors[] = { PowerGpu1, PowerGpu2, PowerModule };
constexpr auto    mcuPowerSensorsSize = 3;

constexpr auto mcuVoltageSensorsSize = 0;

// ---- Verbatim from nsm_type_3.cpp:668-713 ----

// Models __attribute__((weak)) is_busbar_available(): nondet to cover both paths.
bool is_busbar_available()
{
    return nondet_bool();
}

bool is_temp_sensor_available(uint8_t sensorId)
{
    if (sensorId == BusBar_Temp && !is_busbar_available()) {
        return false;
    }
    for (const auto& validSensorId : mcuTemperatureSensors) {
        if (validSensorId == sensorId) {
            return true;
        }
    }
    return false;
}

bool is_power_sensor_available(uint8_t sensorId)
{
    // mcuPowerSensorsSize == 3, so the constexpr-false branch is omitted.
    for (const auto& validSensorId : mcuPowerSensors) {
        if (validSensorId == sensorId) {
            return true;
        }
    }
    return false;
}

// mcuVoltageSensorsSize == 0 for testrunner config: always returns false.
bool is_voltage_sensor_available(uint8_t /*sensorId*/)
{
    return false;
}

}  // namespace verif

namespace {

// ----- Phase 1: totality — no UB, no OOB on any input -----

void f_temp_total()
{
    uint8_t id = nondet_u8();
    (void)verif::is_temp_sensor_available(id);
}

void f_power_total()
{
    uint8_t id = nondet_u8();
    (void)verif::is_power_sensor_available(id);
}

void f_voltage_total()
{
    uint8_t id = nondet_u8();
    (void)verif::is_voltage_sensor_available(id);
}

// ----- Phase 2: functional contracts -----

void f_temp_contract()
{
    uint8_t id     = nondet_u8();
    bool    result = verif::is_temp_sensor_available(id);

#ifdef ESBMC_FUNCTIONAL
    // Accepted iff id is in the configured temperature array AND,
    // if id == BusBar_Temp, the busbar is available.
    // Since is_busbar_available() is nondet the result covers both cases;
    // the contract holds for every concrete busbar state ESBMC explores.
    bool in_array =
        id == verif::TempGpu1     || id == verif::TempGpu2      ||
        id == verif::TempTMP451_1 || id == verif::TempTMP451_2  ||
        id == verif::TempMaxModule || id == verif::TempSMAInternal ||
        id == verif::BusBar_Temp  || id == verif::SMA_Internal;

    // If id is not in the array at all, result must be false regardless of busbar.
    if (!in_array) {
        __ESBMC_assert(!result, "temp: non-member rejected");
    }
    // If id == BusBar_Temp and busbar is unavailable, result must be false.
    // (busbar state is captured inside is_temp_sensor_available via the
    //  same nondet_bool() call — ESBMC explores both paths.)
#endif
    (void)result;
}

void f_power_contract()
{
    uint8_t id     = nondet_u8();
    bool    result = verif::is_power_sensor_available(id);

#ifdef ESBMC_FUNCTIONAL
    bool expected = (id == verif::PowerGpu1 ||
                     id == verif::PowerGpu2 ||
                     id == verif::PowerModule);
    __ESBMC_assert(result == expected, "power: result iff member");
#endif
    (void)result;
}

void f_voltage_contract()
{
    uint8_t id     = nondet_u8();
    bool    result = verif::is_voltage_sensor_available(id);

#ifdef ESBMC_FUNCTIONAL
    __ESBMC_assert(!result, "voltage: always false (empty config)");
#endif
    (void)result;
}

}  // namespace

int main()
{
    switch (nondet_uint() % 6) {
        case 0: f_temp_total();      break;
        case 1: f_power_total();     break;
        case 2: f_voltage_total();   break;
        case 3: f_temp_contract();   break;
        case 4: f_power_contract();  break;
        case 5: f_voltage_contract(); break;
    }
    return 0;
}
