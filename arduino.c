#include <stdio.h>

#include "decode.h"
#include "decode.c"

#include "list.gen.h"

int main()
{
	uint16_t word_n = NUM_WORDS;
	uint32_t idx = 0;
	uint32_t word_int = 0;
	while (word_n--) {
		uint32_t delta;
		idx = nibble_decode(&delta, idx, words_encoded);
		word_int += delta;
		char word[5];
		word[4] = 0;
		u32_to_word(word_int, word);
		printf("%s\n", word);
	}
	return 0;
}
