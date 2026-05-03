// ESBMC verification stub for nv/mctp/nsm_msg_bitmask.h
// Replaces std::array::at() with operator[] throughout so that set_bit /
// get_bit / is_bit_set remain constexpr under ESBMC's bundled <array>, which
// does not mark at() as constexpr (esbmc issue).
#pragma once

#include <array>
#include <cstdint>

namespace nv::mctp::nsm_msg {

static constexpr uint8_t NvMctpSupportedNum      = 32;
static constexpr uint8_t NvMctpEventSupportedNum = 8;
static constexpr uint8_t NsmT5SuppErrorTyesNum   = NvMctpEventSupportedNum;

static constexpr void set_bit(std::array<uint8_t, NvMctpSupportedNum>& bitmask, uint8_t pos)
{
    const size_t byte_index  = pos / 8;
    const size_t bit_offset  = pos % 8;
    bitmask[byte_index]     |= static_cast<uint8_t>((1U << bit_offset) & UINT8_MAX);
}

static constexpr void unset_bit(std::array<uint8_t, NvMctpSupportedNum>& bitmask, uint8_t pos)
{
    const size_t byte_index  = pos / 8;
    const size_t bit_offset  = pos % 8;
    bitmask[byte_index]     &= static_cast<uint8_t>(~((1U << bit_offset) & UINT8_MAX));
}

static constexpr uint8_t get_bit(const std::array<uint8_t, NvMctpSupportedNum>& bitmask,
                                 uint8_t pos)
{
    const size_t byte_index = pos / 8;
    if (byte_index < bitmask.size()) {
        const size_t bit_offset = pos % 8;
        auto mask = static_cast<uint8_t>((1U << bit_offset) & UINT8_MAX);
        return static_cast<uint8_t>(bitmask[byte_index] & mask);
    }
    return 0;
}

[[maybe_unused]] static constexpr bool
is_bit_set(const std::array<uint8_t, NvMctpSupportedNum>& bitmask, uint8_t pos)
{
    return get_bit(bitmask, pos) != 0;
}

static constexpr void set_bit(std::array<uint8_t, NvMctpEventSupportedNum>& bitmask,
                              uint8_t pos)
{
    const size_t byte_index  = pos / 8;
    const size_t bit_offset  = pos % 8;
    bitmask[byte_index]     |= static_cast<uint8_t>((1U << bit_offset) & UINT8_MAX);
}

static constexpr void unset_bit(std::array<uint8_t, NvMctpEventSupportedNum>& bitmask,
                                uint8_t pos)
{
    const size_t byte_index  = pos / 8;
    const size_t bit_offset  = pos % 8;
    bitmask[byte_index]     &= static_cast<uint8_t>(~((1U << bit_offset) & UINT8_MAX));
}

static constexpr uint8_t get_bit(const std::array<uint8_t, NvMctpEventSupportedNum>& bitmask,
                                 uint8_t pos)
{
    const size_t byte_index = pos / 8;
    if (byte_index < bitmask.size()) {
        const size_t bit_offset = pos % 8;
        auto mask = static_cast<uint8_t>((1U << bit_offset) & UINT8_MAX);
        return static_cast<uint8_t>(bitmask[byte_index] & mask);
    }
    return 0;
}

[[maybe_unused]] static constexpr bool
is_bit_set(const std::array<uint8_t, NvMctpEventSupportedNum>& bitmask, uint8_t pos)
{
    return get_bit(bitmask, pos) != 0;
}

}  // namespace nv::mctp::nsm_msg
