#include "decode.h"

uint32_t nibble_decode(uint32_t *value_out, uint32_t idx, const uint8_t *buf)
{
    uint32_t value = 0;
    uint8_t byte;
    do {
        byte = buf[idx >> 1];
        if (idx & 1)
            byte >>= 4;
        value = (value << 3) | (byte & 7);
        value++;
        idx++;
    } while (byte & 0x8);
    *value_out = value;
    return idx;
}

void u32_to_word(uint32_t word_int, char *out)
{
    int i;
    for (i = 0; i < 4; i++) {
        out[i] = value_to_letter[word_int % 26];
        word_int /= 26;
    }
    out[4] = 0;
}
