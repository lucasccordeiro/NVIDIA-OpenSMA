// F-8 failure-mode demonstrator: validateGpioSpoofingErrorInjectionPayload OOB.
//
// Production: validateGpioSpoofingErrorInjectionPayload (nsm_type_5.cpp line 302):
//   for (uint16_t i = 0; i < gpioSpoofingPayload.gpio_spoofing_header.ei_gpio_number; i++) {
//       const auto gpio_index = gpioSpoofingPayload.gpio_ei_entries[i].gpioIndex;
// gpio_ei_entries has MaxGPIOSpoofingEntries = 16 elements.
// The production caller (line 1146) guards: if (ei_gpio_number > 16) return.
// The validator itself has no such guard — calling it directly with
// ei_gpio_number > 16 causes an out-of-bounds read at gpio_ei_entries[16..n-1].
//
// Expected runtime behaviour: AddressSanitizer reports an out-of-bounds read
// at gpio_ei_entries[16] when ei_gpio_number = 17.

#include <cstdint>

extern "C" {
unsigned short __VERIFIER_nondet_ushort(void);
unsigned char  __VERIFIER_nondet_uchar(void);
void           __VERIFIER_assume(int cond);
}

constexpr uint8_t MaxGPIOSpoofingEntries = 16;

struct [[gnu::packed]] GpioSpoofingPayloadHeader {
    uint32_t offset{0};
    uint16_t error_injection_id{0};
    uint16_t ei_gpio_number{0};
};

struct [[gnu::packed]] GpioSpoofingEntry {
    uint16_t gpioIndex : 14;
    uint16_t activated : 1;
    uint16_t polarity  : 1;
};

struct [[gnu::packed]] GPIOSpoofingPayload {
    GpioSpoofingPayloadHeader gpio_spoofing_header{0, 0, 0};
    GpioSpoofingEntry         gpio_ei_entries[MaxGPIOSpoofingEntries]{};
};

// Verbatim from nsm_type_5.cpp lines 298-333 (GPIO driver calls stripped;
// the OOB occurs before any driver call).
static bool validateGpioSpoofingErrorInjectionPayload(GPIOSpoofingPayload& p)
{
    for (uint16_t i = 0; i < p.gpio_spoofing_header.ei_gpio_number; i++) {
        const auto gpio_index = p.gpio_ei_entries[i].gpioIndex;  // OOB when i >= 16
        (void)gpio_index;
    }
    return true;
}

int main()
{
    GPIOSpoofingPayload payload{};

    uint16_t n = __VERIFIER_nondet_ushort();
    __VERIFIER_assume(n > MaxGPIOSpoofingEntries);
    payload.gpio_spoofing_header.ei_gpio_number = n;

    for (uint8_t j = 0; j < MaxGPIOSpoofingEntries; j++) {
        payload.gpio_ei_entries[j].gpioIndex = __VERIFIER_nondet_uchar() & 0x3FFF;
        payload.gpio_ei_entries[j].activated = 0;
        payload.gpio_ei_entries[j].polarity  = 0;
    }

    // Direct call — no caller-side guard on ei_gpio_number.
    validateGpioSpoofingErrorInjectionPayload(payload);

    return 0;
}
