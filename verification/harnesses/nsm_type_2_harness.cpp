// ESBMC harness for nv::mctp::nsm_type2::validatePcieLinkResetValue
// (production: src/nv/mctp/nsm_type_2.{h,cpp}).
//
// nsm_type_2.h pulls nv/mctp/constants.h which transitively drags in
// mbedTLS, gpio, ipc, and an MCU-only NV_IPC_CONFIG_H macro — none of
// which the verified function actually needs. We inline the function
// body and its lookup table verbatim from production. The function and
// table are 12 source lines combined; drift risk is minimal and the
// alternative (stubbing the entire NV IPC + crypto subsystem) is
// disproportionate.
//
// Phase 1: confirm the linear scan is total over uint8_t.
// Phase 2: contract — returns true iff device_index is one of the
//          11 documented valid devices.

#include <cstdint>

extern "C" {
uint8_t  nondet_u8();
unsigned nondet_uint();
}

namespace verif {

// Verbatim from nsm_type_2.h (PciLinksResetDevices enumerators + array).
constexpr uint8_t AllNvswPcieReset = 0x4F;
constexpr uint8_t PexswPcieReset   = 0x50;
constexpr uint8_t Cx9PcieReset_0   = 0x60;
constexpr uint8_t Cx9PcieReset_1   = 0x61;
constexpr uint8_t Cx9PcieReset_2   = 0x62;
constexpr uint8_t Cx9PcieReset_3   = 0x63;
constexpr uint8_t Cx9PcieReset_4   = 0x64;
constexpr uint8_t Cx9PcieReset_5   = 0x65;
constexpr uint8_t Cx9PcieReset_6   = 0x66;
constexpr uint8_t Cx9PcieReset_7   = 0x67;
constexpr uint8_t AllCx9PcieReset  = 0x70;

constexpr uint8_t pciLinkValidDevices[] = {
    AllNvswPcieReset, PexswPcieReset,
    Cx9PcieReset_0, Cx9PcieReset_1, Cx9PcieReset_2, Cx9PcieReset_3,
    Cx9PcieReset_4, Cx9PcieReset_5, Cx9PcieReset_6, Cx9PcieReset_7,
    AllCx9PcieReset,
};

// Verbatim from nsm_type_2.cpp.
bool validatePcieLinkResetValue(uint8_t device_index)
{
    for (const auto& valid : pciLinkValidDevices) {
        if (device_index == valid) {
            return true;
        }
    }
    return false;
}

}  // namespace verif

namespace {

void f_total()
{
    // Phase 1: validatePcieLinkResetValue must terminate without OOB on
    // any uint8_t input.
    uint8_t x = nondet_u8();
    (void)verif::validatePcieLinkResetValue(x);
}

void f_contract_membership()
{
    // Phase 2 forward: every documented enumerator value must be accepted.
    uint8_t v = nondet_u8();
    bool is_valid_value =
        v == verif::AllNvswPcieReset || v == verif::PexswPcieReset ||
        v == verif::Cx9PcieReset_0 || v == verif::Cx9PcieReset_1 ||
        v == verif::Cx9PcieReset_2 || v == verif::Cx9PcieReset_3 ||
        v == verif::Cx9PcieReset_4 || v == verif::Cx9PcieReset_5 ||
        v == verif::Cx9PcieReset_6 || v == verif::Cx9PcieReset_7 ||
        v == verif::AllCx9PcieReset;

#ifdef ESBMC_FUNCTIONAL
    __ESBMC_assert(verif::validatePcieLinkResetValue(v) == is_valid_value,
                   "validate iff member");
#else
    (void)is_valid_value;
#endif
}

void f_contract_rejection()
{
    // Phase 2 reverse: a value not in the table must be rejected.
    // We pick a value strictly less than the smallest enumerator (0x4F),
    // which trivially cannot be in the table.
    uint8_t v = nondet_u8();
    __ESBMC_assume(v < verif::AllNvswPcieReset);  // outside the documented range

#ifdef ESBMC_FUNCTIONAL
    __ESBMC_assert(!verif::validatePcieLinkResetValue(v),
                   "validate rejects below-range");
#endif
    (void)v;
}

}  // namespace

int main()
{
    switch (nondet_uint() % 3) {
        case 0: f_total(); break;
        case 1: f_contract_membership(); break;
        case 2: f_contract_rejection(); break;
    }
    return 0;
}
