// ESBMC negative harness for F-8: validateGpioSpoofingErrorInjectionPayload unbounded loop.
//
// Finding: validateGpioSpoofingErrorInjectionPayload (nsm_type_5.cpp line 302) iterates:
//   for (uint16_t i = 0; i < gpioSpoofingPayload.gpio_spoofing_header.ei_gpio_number; i++) {
//       const auto gpio_index = gpioSpoofingPayload.gpio_ei_entries[i].gpioIndex;
//
// The array gpio_ei_entries has MaxGPIOSpoofingEntries = 16 elements.
// The production caller (line 1146) guards: if (gpioSpoofingHeader.ei_gpio_number > 16) return.
// But validateGpioSpoofingErrorInjectionPayload itself has no such guard; calling it directly
// with ei_gpio_number > 16 causes OOB access on gpio_ei_entries[i] for i >= 16.
//
// This harness calls the validator directly (bypassing the caller's pre-filter) with
// ei_gpio_number > MaxGPIOSpoofingEntries.
//
// Expected: VERIFICATION FAILED — OOB access on gpio_ei_entries[i] for i >= 16.

#include <cstdint>

extern "C" {
uint8_t  nondet_u8();
uint16_t nondet_u16();
}

// Verbatim from nsm_type_5.h
constexpr uint8_t MaxGPIOSpoofingEntries = 16;

struct [[gnu::packed]] GpioSpoofingPayloadHeader
{
    uint32_t offset;
    uint16_t error_injection_id;
    uint16_t ei_gpio_number;
};

struct [[gnu::packed]] GpioSpoofingEntry
{
    uint16_t gpioIndex : 14;
    uint16_t activated : 1;
    uint16_t polarity  : 1;
};

struct [[gnu::packed]] GPIOSpoofingPayload
{
    GpioSpoofingPayloadHeader gpio_spoofing_header{0, 0, 0};
    GpioSpoofingEntry         gpio_ei_entries[MaxGPIOSpoofingEntries]{};
};

// Verbatim from nsm_type_5.cpp line 298-333 (stripped of GPIO driver calls
// that require platform stubs; the OOB occurs before any GPIO driver call).
static bool validateGpioSpoofingErrorInjectionPayload(GPIOSpoofingPayload& gpioSpoofingPayload)
{
    for (uint16_t i = 0; i < gpioSpoofingPayload.gpio_spoofing_header.ei_gpio_number; i++) {
        // OOB when i >= MaxGPIOSpoofingEntries and ei_gpio_number > MaxGPIOSpoofingEntries
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
        const auto gpio_index = gpioSpoofingPayload.gpio_ei_entries[i].gpioIndex;
        (void)gpio_index;
        // (remaining GPIO-driver checks omitted — OOB manifests on the array access above)
    }
    return true;
}

int main()
{
    GPIOSpoofingPayload payload{};

    // Set ei_gpio_number > MaxGPIOSpoofingEntries to bypass the production caller guard
    // and trigger OOB directly in the validator.
    uint16_t n = nondet_u16();
    __ESBMC_assume(n > MaxGPIOSpoofingEntries);  // forces i to exceed array bound
    payload.gpio_spoofing_header.ei_gpio_number = n;

    // Populate valid entries up to MaxGPIOSpoofingEntries (the rest are OOB)
    for (uint8_t j = 0; j < MaxGPIOSpoofingEntries; j++) {
        payload.gpio_ei_entries[j].gpioIndex = nondet_u8();
        payload.gpio_ei_entries[j].activated = 0;
        payload.gpio_ei_entries[j].polarity  = 0;
    }

    // Direct call to the validator — no caller-side bounds check on ei_gpio_number.
    (void)validateGpioSpoofingErrorInjectionPayload(payload);

    return 0;
}
