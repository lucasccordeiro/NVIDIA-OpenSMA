// ESBMC harness for nsm_type5 field-validation functions
// (production: src/nv/mctp/nsm_type_5.cpp).
//
// The validation functions pull in a large transitive dependency tree
// (FreeRTOS IPC, GPIO, I2C, logger, boot) so we inline their bodies and
// the required constants verbatim. The functions are 1-6 source lines each;
// drift risk is minimal and the alternative (stubbing the entire NV subsystem)
// is disproportionate.
//
// Functions verified:
//   validateFatalErrorInjectionPayload(uint32_t fault_bitmap)
//   validateDeviceIndexGpuDegradeMode(uint8_t device_index)
//   validateActionGpuDegradeMode(uint8_t action)
//   validateDeviceIndexPowerSupply(uint8_t device_index)
//   validateModePowerSupply(uint8_t mode)
//
// Phase 1: each function is total over its full input domain (no UB, no OOB).
// Phase 2: functional contracts — result iff documented acceptance condition.

#include <cstdint>

extern "C" {
uint8_t  nondet_u8();
uint32_t nondet_u32();
unsigned nondet_uint();
}

namespace verif {

// ---- Constants (verbatim from nsm_type_5.h / nsm_type_5.cpp) ----

// FatalFaultEIPayloadValues: bit positions
constexpr uint32_t FatalFaultEIMCUException    = 0;
constexpr uint32_t FatalFaultEIWatchdogTimeout = 1;
constexpr uint32_t FatalFaultEIMask =
    (1u << FatalFaultEIMCUException) | (1u << FatalFaultEIWatchdogTimeout);

// NsmDevCfgEnablingMode enumerators
constexpr uint8_t ModeDisable = 0x00;
constexpr uint8_t ModeEnable  = 0x01;

// GPU degrade mode device index range
constexpr uint8_t lowDeviceIndexGpuDegradeMode  = 0x80;
constexpr uint8_t highDeviceIndexGpuDegradeMode = 0x87;

// Power supply device index upper bound
constexpr uint8_t highDeviceIndexPowerSupply = 0x07;

// ---- Functions (verbatim from nsm_type_5.cpp) ----

bool validateFatalErrorInjectionPayload(uint32_t fault_bitmap)
{
    const auto bitset_counter = __builtin_popcount(fault_bitmap);
    if (bitset_counter > 0) {
        if (bitset_counter > 1)
            return false;
        if ((fault_bitmap & FatalFaultEIMask) == 0)
            return false;
    }
    return true;
}

bool validateDeviceIndexGpuDegradeMode(uint8_t device_index)
{
    if (device_index < lowDeviceIndexGpuDegradeMode
        || device_index > highDeviceIndexGpuDegradeMode)
        return false;
    return true;
}

bool validateActionGpuDegradeMode(uint8_t action)
{
    if (action != ModeDisable && action != ModeEnable)
        return false;
    return true;
}

bool validateDeviceIndexPowerSupply(uint8_t device_index)
{
    if (device_index > highDeviceIndexPowerSupply)
        return false;
    return true;
}

bool validateModePowerSupply(uint8_t mode)
{
    if (mode != ModeDisable && mode != ModeEnable)
        return false;
    return true;
}

}  // namespace verif

namespace {

// ----- Phase 1: totality -----

void f_fatal_ei_total()
{
    uint32_t bm = nondet_u32();
    (void)verif::validateFatalErrorInjectionPayload(bm);
}

void f_gpu_degrade_idx_total()
{
    uint8_t d = nondet_u8();
    (void)verif::validateDeviceIndexGpuDegradeMode(d);
}

void f_gpu_degrade_action_total()
{
    uint8_t a = nondet_u8();
    (void)verif::validateActionGpuDegradeMode(a);
}

void f_power_supply_idx_total()
{
    uint8_t d = nondet_u8();
    (void)verif::validateDeviceIndexPowerSupply(d);
}

void f_power_supply_mode_total()
{
    uint8_t m = nondet_u8();
    (void)verif::validateModePowerSupply(m);
}

// ----- Phase 2: functional contracts -----

void f_fatal_ei_contract()
{
    uint32_t bm = nondet_u32();
    bool result = verif::validateFatalErrorInjectionPayload(bm);

#ifdef ESBMC_FUNCTIONAL
    // Characterise accepted values:
    //   zero bitmap                          → accept (no fault requested)
    //   exactly one bit, that bit in mask    → accept (single valid fault)
    //   all other bitmaps                    → reject
    const int  pc        = __builtin_popcount(bm);
    const bool in_mask   = (bm & verif::FatalFaultEIMask) != 0;
    const bool expected  = (bm == 0) || (pc == 1 && in_mask);
    __ESBMC_assert(result == expected, "validateFatalErrorInjectionPayload contract");
#endif
}

void f_gpu_degrade_idx_contract()
{
    uint8_t d      = nondet_u8();
    bool    result = verif::validateDeviceIndexGpuDegradeMode(d);
#ifdef ESBMC_FUNCTIONAL
    bool expected = (d >= verif::lowDeviceIndexGpuDegradeMode
                  && d <= verif::highDeviceIndexGpuDegradeMode);
    __ESBMC_assert(result == expected, "validateDeviceIndexGpuDegradeMode contract");
#endif
}

void f_gpu_degrade_action_contract()
{
    uint8_t a      = nondet_u8();
    bool    result = verif::validateActionGpuDegradeMode(a);
#ifdef ESBMC_FUNCTIONAL
    bool expected = (a == verif::ModeDisable || a == verif::ModeEnable);
    __ESBMC_assert(result == expected, "validateActionGpuDegradeMode contract");
#endif
}

void f_power_supply_idx_contract()
{
    uint8_t d      = nondet_u8();
    bool    result = verif::validateDeviceIndexPowerSupply(d);
#ifdef ESBMC_FUNCTIONAL
    bool expected = (d <= verif::highDeviceIndexPowerSupply);
    __ESBMC_assert(result == expected, "validateDeviceIndexPowerSupply contract");
#endif
}

void f_power_supply_mode_contract()
{
    uint8_t m      = nondet_u8();
    bool    result = verif::validateModePowerSupply(m);
#ifdef ESBMC_FUNCTIONAL
    bool expected = (m == verif::ModeDisable || m == verif::ModeEnable);
    __ESBMC_assert(result == expected, "validateModePowerSupply contract");
#endif
}

}  // namespace

int main()
{
    switch (nondet_uint() % 10) {
        case 0: f_fatal_ei_total();               break;
        case 1: f_gpu_degrade_idx_total();        break;
        case 2: f_gpu_degrade_action_total();     break;
        case 3: f_power_supply_idx_total();       break;
        case 4: f_power_supply_mode_total();      break;
        case 5: f_fatal_ei_contract();            break;
        case 6: f_gpu_degrade_idx_contract();     break;
        case 7: f_gpu_degrade_action_contract();  break;
        case 8: f_power_supply_idx_contract();    break;
        case 9: f_power_supply_mode_contract();   break;
    }
    return 0;
}
