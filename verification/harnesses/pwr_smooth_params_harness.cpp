// ESBMC harness for soc_pwr_smoothing wire-format helpers and param-id validator.
//
// Functions verified (inlined verbatim from presets.h / presets.cpp):
//   OverrideParam::to_uint32()   — pack {is_override, reserved, value} into uint32_t
//   OverrideParam::from_uint32() — unpack uint32_t → {is_override, reserved, value}
//   is_valid_param_id(uint8_t)   — param_id ∈ [0, 27] ∪ [40, 44]
//
// Phase 1: no UB on any input.
// Phase 2: functional contracts —
//   - round-trip pack:   from_uint32(p.to_uint32()) == p  for all OverrideParam
//   - round-trip unpack: to_uint32(from_uint32(raw)) == raw for all uint32_t
//   - is_valid_param_id: true iff param_id < MaxTuningParams (28)
//                        OR (TestHookParamsStart (40) <= param_id < MaxParamCount (45))

#include <cstdint>

extern "C" {
uint8_t  nondet_u8();
uint16_t nondet_u16();
uint32_t nondet_u32();
unsigned nondet_uint();
}

namespace verif {

// ---- RackPwrSmoothParams enumerators (verbatim from nv/mctp/nsm_type_ff.h) ----
constexpr uint8_t MaxTuningParams      = 28;
constexpr uint8_t TestHookParamsStart  = 40;
constexpr uint8_t MaxParamCount        = 45;

// ---- Wire-format constants (verbatim from soc_pwr_smoothing/presets.cpp) ----
constexpr uint32_t kOverrideIsOverrideShift = 24U;
constexpr uint32_t kOverrideReservedShift   = 16U;
constexpr uint32_t kByteMask                = 0xFFU;
constexpr uint32_t kUint16Mask              = 0xFFFFU;

// ---- OverrideParam struct + methods (verbatim from soc_pwr_smoothing/presets.h/.cpp) ----
struct OverrideParam
{
    uint8_t  is_override;
    uint8_t  reserved;
    uint16_t value;

    uint32_t to_uint32() const
    {
        return (static_cast<uint32_t>(is_override) << kOverrideIsOverrideShift)
             | (static_cast<uint32_t>(reserved) << kOverrideReservedShift)
             | static_cast<uint32_t>(value);
    }

    static OverrideParam from_uint32(uint32_t raw)
    {
        return OverrideParam{
            .is_override = static_cast<uint8_t>((raw >> kOverrideIsOverrideShift) & kByteMask),
            .reserved    = static_cast<uint8_t>((raw >> kOverrideReservedShift) & kByteMask),
            .value       = static_cast<uint16_t>(raw & kUint16Mask)};
    }
};

// ---- is_valid_param_id (verbatim from soc_pwr_smoothing/presets.h) ----
constexpr bool is_valid_param_id(uint8_t param_id)
{
    return (param_id < MaxTuningParams)
        || (param_id >= TestHookParamsStart && param_id < MaxParamCount);
}

}  // namespace verif

namespace {

// ----- Phase 1: totality -----

void f_to_uint32()
{
    verif::OverrideParam p{nondet_u8(), nondet_u8(), nondet_u16()};
    (void)p.to_uint32();
}

void f_from_uint32()
{
    uint32_t raw = nondet_u32();
    (void)verif::OverrideParam::from_uint32(raw);
}

void f_is_valid()
{
    uint8_t id = nondet_u8();
    (void)verif::is_valid_param_id(id);
}

// ----- Phase 2: functional contracts -----

void f_round_trip_pack()
{
    verif::OverrideParam p{nondet_u8(), nondet_u8(), nondet_u16()};
    verif::OverrideParam q = verif::OverrideParam::from_uint32(p.to_uint32());

#ifdef ESBMC_FUNCTIONAL
    __ESBMC_assert(q.is_override == p.is_override && q.reserved == p.reserved
                       && q.value == p.value,
                   "round-trip: from_uint32(to_uint32(p)) == p");
#endif
    (void)q;
}

void f_round_trip_unpack()
{
    uint32_t raw  = nondet_u32();
    uint32_t raw2 = verif::OverrideParam::from_uint32(raw).to_uint32();

#ifdef ESBMC_FUNCTIONAL
    __ESBMC_assert(raw2 == raw, "round-trip: to_uint32(from_uint32(raw)) == raw");
#endif
    (void)raw2;
}

void f_valid_param_id_contract()
{
    uint8_t id = nondet_u8();
    bool    v  = verif::is_valid_param_id(id);

#ifdef ESBMC_FUNCTIONAL
    bool expected = (id < verif::MaxTuningParams)
                 || (id >= verif::TestHookParamsStart && id < verif::MaxParamCount);
    __ESBMC_assert(v == expected, "is_valid_param_id: exact characterisation");
#endif
    (void)v;
}

}  // namespace

int main()
{
    switch (nondet_uint() % 6) {
        case 0: f_to_uint32();               break;
        case 1: f_from_uint32();             break;
        case 2: f_is_valid();                break;
        case 3: f_round_trip_pack();         break;
        case 4: f_round_trip_unpack();       break;
        case 5: f_valid_param_id_contract(); break;
    }
    return 0;
}
