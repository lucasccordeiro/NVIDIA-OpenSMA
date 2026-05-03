// DCD GPIO safety: on_dcd_get_gpio and on_dcd_set_gpio bounds proof
//
// nsm.cpp:3389-3479 (get) and 3482-3575 (set).
// Both handlers guard:
//   if ((offset + length) > GpioNum) → reject (ErrorInvalidData)
// then loop for i ∈ [0, length):
//   gpio_index = offset + i  →  GpioSetup.at(gpio_index)
//   byte_index = i / 8       →  gpio_resp.gpio.at(byte_index)
//                             →  gpio_req.gpio.at(byte_index)   [set path]
//
// Invariants after guard:
//   gpio_index = offset + i < offset + length ≤ GpioNum = GpioSetup.size()
//   byte_index = i/8 ≤ (GpioNum−1)/8 < GpioBytes = gpio_resp.gpio.size()
//
// Representative production value: GpioNum = 66 (p3957_cxx: 8+11+10+18+11+8).
// GpioBytes = (66+7)/8 = 9.
//
// Expected: VERIFICATION SUCCESSFUL — all .at() calls provably in bounds.

#include <array>
#include <cstdint>

extern "C" { uint16_t nondet_u16(); unsigned nondet_uint(); }

static constexpr uint16_t GpioNum  = 66;
static constexpr uint16_t GpioBytes = (GpioNum + 7) / 8;  // = 9

int main()
{
    uint16_t offset = nondet_u16();
    uint16_t length = nondet_u16();

    // Model the guard present in both handlers (uint16_t promotes to int — no overflow)
    if ((offset + length) > GpioNum) {
        return 0;
    }

    // GpioSetup: std::array<stubT, GpioNum> — only array bounds matter here
    std::array<uint8_t, GpioNum>   GpioSetup{};
    std::array<uint8_t, GpioBytes> gpio_resp_gpio{};
    std::array<uint8_t, GpioBytes> gpio_req_gpio{};

    // get path: discover-all special case sets length = GpioNum after guard
    bool get_path = (nondet_uint() % 2 == 0);
    if (get_path && offset == 0 && length == 0) {
        length = GpioNum;
    }

    // Main loop (models both get and set paths)
    for (uint16_t i = 0; i < length; i++) {
        const uint16_t gpio_index = offset + i;
        const uint16_t byte_index = i / 8;
        const uint16_t bit_pos    = i % 8;

        (void)(GpioSetup.at(gpio_index));                          // nsm.cpp:3446, 3541
        gpio_resp_gpio.at(byte_index) |= (1U << bit_pos);         // nsm.cpp:3461, 3560
        (void)(gpio_req_gpio.at(byte_index));                      // nsm.cpp:3546 (set path)
    }

    // Log loop: byte_length = (length + 7) / 8 ≤ GpioBytes  →  nsm.cpp:3467, 3566
    const uint16_t byte_length = (length + 7) / 8;
    for (uint16_t i = 0; i < byte_length; i++) {
        (void)(gpio_resp_gpio.at(i));
    }

    return 0;
}
