#include <assert.h>
#include <error.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

////////////////////////////
// xorshift128+ generator //
////////////////////////////
static uint64_t s[ 2 ];

static uint64_t xrand(void) {
    uint64_t s1 = s[ 0 ];
    const uint64_t s0 = s[ 1 ];
    s[ 0 ] = s0;
    s1 ^= s1 << 23; // a
    return ( s[ 1 ] = ( s1 ^ s0 ^ ( s1 >> 17 ) ^ ( s0 >> 26 ) ) ) + s0; // b, c
}
////////////////////////////

static int int_compar(const void *a, const void *b)
{
    return *(int*)a - *(int*)b;
}

#define NUM_WORDS 5526

char words[NUM_WORDS][5];

uint8_t letter_to_value[26];
uint8_t value_to_letter[26];

static int word_to_int(char *word)
{
    int i;
    int ret = 0;
    for (i = 3; i >= 0; i--)
        ret = (ret * 26) + letter_to_value[word[i] - 'A'];
    return ret;
}

static void build_rev(void)
{
    int i;
    for (i = 0; i < 26; i++) {
        value_to_letter[letter_to_value[i]] = i;
    }
}

static void int_to_word(int word_int, char *out)
{
    build_rev();
    int i;
    for (i = 0; i < 4; i++) {
        out[i] = value_to_letter[word_int % 26] + 'A';
        word_int /= 26;
    }
    out[4] = 0;
}

// Initialize buf to a permutation -- each value in [0, len) is present
static void gen_permutation(uint8_t *buf, int len)
{
    int i, j;
    for (i = 0; i < len; i++) {
        j = xrand() % (i + 1);
        if (j != i)
            buf[i] = buf[j];
        buf[j] = i;
    }
}

// compute size of value encoded with variable-nibble format
//
// Bit pattern            Values
// 0xxx:                    0-7
// 1xxx0xxx:                8-71
// 1xxx1xxx0xxx:           72-583
// 1xxx1xxx1xxx0xxx:      584-4679
// 1xxx1xxx1xxx1xxx0xxx: 4680-37448
static unsigned nibble_encoded_size(int delta)
{
    if (delta < 0b1000)
        return 4;
    else if (delta < 0b1001000)
        return 8;
    else if (delta < 0b1001001000)
        return 12;
    else if (delta < 0b1001001001000)
        return 16;
    else if (delta < 0b1001001001001000)
        return 20;
    else if (delta < 0b1001001001001001000)
        return 24;
    return 10000;
}

// determine how many bits it would take to encode the words
// with the current letter_to_value
static unsigned compute_efficiency(void)
{
    /* convert words to ints */
    int i;
    int word_values[NUM_WORDS];
    for (i = 0; i < NUM_WORDS; i++)
        word_values[i] = word_to_int(words[i]);
    qsort(word_values, NUM_WORDS, sizeof(int), int_compar);

    /* compress list of strictly increasing integers by
     * encoding difference from previous word into nibbles*/
    int last = 0;
    unsigned bits = 0;
    for (i = 0; i < NUM_WORDS; i++) {
        int delta = word_values[i] - last - 1;
        last = word_values[i];
        bits += nibble_encoded_size(delta);
    }

    return bits;
}

int main(void)
{
    int i;
    FILE *wordlist = fopen("list.txt", "r");
    for (i = 0; i < NUM_WORDS; i++)
        fscanf(wordlist, " %4s ", words[i]);
    fclose(wordlist);

    FILE *rand = fopen("/dev/urandom", "r");
    fread(&s, sizeof(uint64_t), 2, rand);
    fclose(rand);

    gen_permutation(letter_to_value, 26);

    for (i = 0; i < NUM_WORDS; i++) {
        char word_again[5];
        int word_val = word_to_int(words[i]);
        int_to_word(word_val, word_again);
        assert(!strcmp(words[i], word_again));
    }

    unsigned best = -1;
    long long iter = 0;
    while (1) {
        gen_permutation(letter_to_value, 26);
        unsigned bits = compute_efficiency();
        unsigned bytes = (bits + 7) / 8 + 26;
        iter++;
        if (!(iter & 0xFFFFF)) {
            printf("iters: %lld\n", iter);
        }
        if (bytes <= best) {
            build_rev();
            for (i = 0; i < 26; i++)
                printf("%c", 'a' + value_to_letter[i]);
            printf("  bytes: %d  iters: %lld\n", bytes, iter);

            best = bytes;
            iter = 0;
        }
    }
}
