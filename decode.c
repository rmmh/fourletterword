#include "decode.h"

uint16_t nibble_decode(uint16_t *value_out, uint16_t idx, const uint8_t *buf)
{
    uint16_t value = 0;
    uint8_t byte;
    buf += idx >> 1;
    do {
        byte = *buf;
        if (idx & 1) {
            byte >>= 4;
            buf++;
        }
        idx++;
        value = (value << 3) | (byte & 7);
        value++;
    } while (byte & 0x8);
    *value_out = value;
    return idx;
}

void u32_to_word(uint32_t word_int, char *out)
{
    uint8_t i;
    for (i = 0; i < 4; i++) {
        out[i] = value_to_letter[word_int % 26];
        word_int /= 26;
    }
    out[4] = 0;
}
