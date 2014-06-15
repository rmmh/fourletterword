#include <stdint.h>

#define NUM_WORDS 5526

#ifdef __AVR_ARCH__
#include <avr/pgmspace.h>
#define DATA PROGMEM const
#else
#define DATA
#endif

uint16_t nibble_decode(uint16_t *value_out, uint16_t nibble, const uint8_t *buf);
void u32_to_word(uint32_t word_int, char *out);


extern DATA uint8_t value_to_letter[26];
