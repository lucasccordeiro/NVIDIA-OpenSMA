// ESBMC harness for src/nv/telemetry/utils.h
//   getTelemIdFromTempSensorId, getTelemIdFromPowerSensorId, buffer_to_uint32
//
// buffer_to_uint32 has a production bug: the last byte is shifted as int
// (signed overflow UB for buffer[3] >= 0x80).  This harness verifies the
// FIXED version; the buggy production form is in telemetry_negative_harness.cpp.
//
// The constexpr lookup functions pull in nv/mctp/enums.h which has no MCU
// dependency at the enum-definition level, but the full header chain is
// disproportionate to stub.  We inline the mapping arrays and function bodies
// verbatim (30 source lines); drift risk is minimal.
//
// Phase 1: all three functions terminate without UB on any input.
// Phase 2: functional contracts —
//   - getTelemIdFromTempSensor/PowerSensor: accepted iff in the mapping table.
//   - buffer_to_uint32: result == LE decoding of the four input bytes.

#include <cstdint>

extern "C" {
uint8_t  nondet_u8();
uint32_t nondet_u32();
unsigned nondet_uint();
}

namespace verif {

// ---- TelemId enumerators (verbatim from src/nv/telemetry/utils.h) ----
enum TelemId : uint8_t
{
    Gpu1Temp     = 0,
    Gpu2Temp     = 1,
    Gpu1Power    = 2,
    Gpu2Power    = 3,
    ModulePower  = 4,
    ModuleTemp1  = 5,
    ModuleTemp2  = 6,
    InternalTemp = 7,
    MaxModuleTemp = 8,
    Gpio         = 9,
    CX8_1_Temp   = 10,
    CX8_2_Temp   = 11,
    QM4_1_Temp   = 12,
    QM4_2_Temp   = 13,
    MaxItem      = 14,
};

// ---- Type3TemperatureSensors / Type3PowerSensors enumerators ----
// (verbatim from src/nv/mctp/enums.h)
constexpr uint8_t TempGpu1        = 0;
constexpr uint8_t TempGpu2        = 1;
constexpr uint8_t TempTMP451_1    = 2;
constexpr uint8_t TempTMP451_2    = 3;
constexpr uint8_t TempMaxModule   = 4;
constexpr uint8_t TempSMAInternal = 5;
constexpr uint8_t TempCX8_1       = 6;
constexpr uint8_t TempCX8_2       = 7;
constexpr uint8_t QM4_1_Temp_id   = 128;
constexpr uint8_t QM4_2_Temp_id   = 129;

constexpr uint8_t PowerGpu1   = 0;
constexpr uint8_t PowerGpu2   = 1;
constexpr uint8_t PowerModule = 2;

// ---- Mapping tables (verbatim from src/nv/telemetry/utils.h) ----
struct TempMapping { uint8_t sensor; TelemId telem; };
constexpr TempMapping TempSensorMap[10] = {
    { TempGpu1,        Gpu1Temp     },
    { TempGpu2,        Gpu2Temp     },
    { TempTMP451_1,    ModuleTemp1  },
    { TempTMP451_2,    ModuleTemp2  },
    { TempMaxModule,   MaxModuleTemp },
    { TempSMAInternal, InternalTemp },
    { TempCX8_1,       CX8_1_Temp   },
    { TempCX8_2,       CX8_2_Temp   },
    { QM4_1_Temp_id,   QM4_1_Temp   },
    { QM4_2_Temp_id,   QM4_2_Temp   },
};

struct PowerMapping { uint8_t sensor; TelemId telem; };
constexpr PowerMapping PowerSensorMap[3] = {
    { PowerGpu1,   Gpu1Power   },
    { PowerGpu2,   Gpu2Power   },
    { PowerModule, ModulePower },
};

// ---- Functions (verbatim from src/nv/telemetry/utils.h) ----

TelemId getTelemIdFromTempSensorId(uint8_t type3Sensor)
{
    for (const auto& mapping : TempSensorMap) {
        if (mapping.sensor == type3Sensor) {
            return mapping.telem;
        }
    }
    return MaxItem;
}

TelemId getTelemIdFromPowerSensorId(uint8_t type3PowerSensor)
{
    for (const auto& mapping : PowerSensorMap) {
        if (mapping.sensor == type3PowerSensor) {
            return mapping.telem;
        }
    }
    return MaxItem;
}

// FIXED version of buffer_to_uint32 (verbatim from src/nv/telemetry/utils.cpp
// except the cast on byte 3 is corrected — see telemetry_negative_harness.cpp
// for the buggy production form that triggers F-6).
uint32_t buffer_to_uint32(const uint8_t b[4])
{
    constexpr uint8_t Byte0 = 0;
    constexpr uint8_t Byte1 = 8;
    constexpr uint8_t Byte2 = 16;
    constexpr uint8_t Byte3 = 24;
    return (static_cast<uint32_t>(b[0]) << Byte0)
         | (static_cast<uint32_t>(b[1]) << Byte1)
         | (static_cast<uint32_t>(b[2]) << Byte2)
         | (static_cast<uint32_t>(b[3]) << Byte3);  // fix: cast b[3] before shifting
}

}  // namespace verif

namespace {

// ----- Phase 1: totality — no UB on any input -----

void f_temp_lookup_total()
{
    uint8_t id = nondet_u8();
    (void)verif::getTelemIdFromTempSensorId(id);
}

void f_power_lookup_total()
{
    uint8_t id = nondet_u8();
    (void)verif::getTelemIdFromPowerSensorId(id);
}

void f_buffer_total()
{
    uint8_t b[4] = { nondet_u8(), nondet_u8(), nondet_u8(), nondet_u8() };
    (void)verif::buffer_to_uint32(b);
}

// ----- Phase 2: functional contracts -----

void f_temp_lookup_contract()
{
    uint8_t  id     = nondet_u8();
    auto     result = verif::getTelemIdFromTempSensorId(id);

#ifdef ESBMC_FUNCTIONAL
    bool in_map = (id == verif::TempGpu1      || id == verif::TempGpu2      ||
                   id == verif::TempTMP451_1  || id == verif::TempTMP451_2  ||
                   id == verif::TempMaxModule  || id == verif::TempSMAInternal ||
                   id == verif::TempCX8_1     || id == verif::TempCX8_2     ||
                   id == verif::QM4_1_Temp_id || id == verif::QM4_2_Temp_id);
    if (!in_map) {
        __ESBMC_assert(result == verif::MaxItem, "temp: non-member → MaxItem");
    } else {
        __ESBMC_assert(result != verif::MaxItem, "temp: member → valid TelemId");
    }
#endif
    (void)result;
}

void f_power_lookup_contract()
{
    uint8_t id     = nondet_u8();
    auto    result = verif::getTelemIdFromPowerSensorId(id);

#ifdef ESBMC_FUNCTIONAL
    bool in_map = (id == verif::PowerGpu1 ||
                   id == verif::PowerGpu2 ||
                   id == verif::PowerModule);
    bool expected_result = in_map
        ? (id == verif::PowerGpu1   ? result == verif::Gpu1Power
         : id == verif::PowerGpu2   ? result == verif::Gpu2Power
                                    : result == verif::ModulePower)
        : (result == verif::MaxItem);
    __ESBMC_assert(expected_result, "power: result matches mapping");
#endif
    (void)result;
}

void f_buffer_contract()
{
    uint8_t b[4] = { nondet_u8(), nondet_u8(), nondet_u8(), nondet_u8() };
    uint32_t result = verif::buffer_to_uint32(b);

#ifdef ESBMC_FUNCTIONAL
    // LE decoding: byte 0 is least significant
    uint32_t expected = (static_cast<uint32_t>(b[0]))
                      | (static_cast<uint32_t>(b[1]) << 8)
                      | (static_cast<uint32_t>(b[2]) << 16)
                      | (static_cast<uint32_t>(b[3]) << 24);
    __ESBMC_assert(result == expected, "buffer_to_uint32 LE decoding");
#endif
    (void)result;
}

}  // namespace

int main()
{
    switch (nondet_uint() % 6) {
        case 0: f_temp_lookup_total();    break;
        case 1: f_power_lookup_total();   break;
        case 2: f_buffer_total();         break;
        case 3: f_temp_lookup_contract(); break;
        case 4: f_power_lookup_contract(); break;
        case 5: f_buffer_contract();      break;
    }
    return 0;
}
