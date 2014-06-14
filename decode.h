#include <stdint.h>

#define NUM_WORDS 5526

#ifdef __AVR_ARCH__
#include <avr/pgmspace.h>
#define DATA PROGMEM const
#else
#define DATA
#endif

uint32_t nibble_decode(uint32_t *value_out, uint32_t nibble, const uint8_t *buf);
void u32_to_word(uint32_t word_int, char *out);


extern DATA uint8_t value_to_letter[26];
