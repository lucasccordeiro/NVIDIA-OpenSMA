// Verification stub for sys::adc::ADC.
// pop_fifo_conv_value returns a nondet uint16_t so ESBMC explores all ADC readings.
// All other methods are no-ops; resolution_bits stays constexpr to satisfy static_asserts
// in StateOfChargeDev.
#pragma once
#include <cstdint>

extern "C" uint16_t nondet_u16();

namespace sys::adc {

using AdcPeripheral = uint32_t;
using AdcChannel    = uint32_t;

constexpr uint8_t ADC_RESOLUTION_HIGH = 0;

class ADC
{
public:
    static constexpr uint32_t resolution_bits(bool hi_resolution, bool /*single_ended*/)
    {
        return hi_resolution ? 16 : 12;
    }

    template<AdcPeripheral /*peripheral*/, uint32_t /*fifo_num*/>
    static uint16_t pop_fifo_conv_value()
    {
        return nondet_u16();
    }

    static bool     init_adc(AdcPeripheral)                    { return true;  }
    static void     trigger_read(AdcPeripheral, uint32_t)      {}
    static uint32_t get_fifo_count(AdcPeripheral, uint8_t)     { return 0; }
    static bool     pop_fifo(AdcPeripheral, uint8_t, uint16_t&, uint32_t&) { return false; }
    static bool     adc_ready(AdcPeripheral)                   { return true;  }
    static uint32_t get_status_flags(AdcPeripheral)            { return 0; }
    static void     reset_fifo(AdcPeripheral, uint8_t)         {}
    static void     enable_vref()                              {}
    static void     enable_adc(AdcPeripheral, bool)            {}
    static void     enable_adc_nvic_interrupt(AdcPeripheral)   {}
    static void     enable_adc_lpadc_interrupt(AdcPeripheral, uint32_t) {}
    static void     disable_adc_nvic_interrupt(AdcPeripheral)  {}
    static void     disable_adc_lpadc_interrupt(AdcPeripheral, uint32_t) {}
    static void     clear_status_flags(AdcPeripheral, uint32_t) {}

    struct AdcConvResult
    {
        uint32_t commandIdSource;
        uint32_t loopCountIndex;
        uint32_t triggerIdSource;
        uint16_t convValue;
    };
    static bool get_adc_reading(AdcPeripheral, AdcConvResult&, uint8_t) { return false; }

    struct AdcCommandConfig
    {
        uint8_t  sampleChannelMode;
        uint8_t  channelNumber;
        uint8_t  chainedNextCommandNumber;
        uint16_t hwCompareValueHigh;
        uint16_t hwCompareValueLow;
        uint8_t  loopCount;
        uint8_t  sampleTimeMode;
        uint8_t  hardwareAverageMode;
        uint8_t  conversionResolutionMode;
        uint8_t  hardwareCompareMode;
    };
    static void set_adc_command(AdcPeripheral, uint32_t, const AdcCommandConfig&) {}

    static constexpr float get_temp_parameter_a()     { return 1.0f; }
    static constexpr float get_temp_parameter_b()     { return 0.0f; }
    static constexpr float get_temp_parameter_alpha() { return 1.0f; }
};

}  // namespace sys::adc
