#pragma once
#include <cstddef>
#include <cstdint>

// The current EXI device is fixed at 4 Mbit. Reject larger images rather than
// silently truncate them. Validate the standard card metadata before replacing
// the user's card; this does not inspect or decode any game's save payload.
inline bool ios_valid_save_card(const uint8_t* card, size_t size) {
    if (!card || size != 524288) return false;
    auto word = [&](size_t at) -> uint16_t {
        return uint16_t((uint16_t(card[at]) << 8) | card[at + 1]);
    };
    if (word(0x22) != 4 || word(0x24) != 0) return false;
    auto checksum = [&](size_t at, size_t length, size_t stored) {
        uint16_t sum = 0, inverse = 0;
        for (size_t i = at; i < at + length; i += 2) {
            uint16_t value = word(i);
            sum = uint16_t(sum + value);
            inverse = uint16_t(inverse + uint16_t(~value));
        }
        if (sum == 0xffff) sum = 0;
        if (inverse == 0xffff) inverse = 0;
        return sum == word(stored) && inverse == word(stored + 2);
    };
    return checksum(0, 0x1fc, 0x1fc)
        && (checksum(8192, 0x1ffc, 8192 + 0x1ffc)
            || checksum(16384, 0x1ffc, 16384 + 0x1ffc))
        && (checksum(24576 + 4, 8188, 24576)
            || checksum(32768 + 4, 8188, 32768));
}
