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

static void build_rev(void)
{
    int i;
    for (i = 0; i < 26; i++) {
        value_to_letter[letter_to_value[i]] = i;
    }
}

static void load_from_value_to_letter(const char *perm)
{
    int i;
    assert(strlen(perm) == 26);
    for (i = 0; i < 26; i++) {
        assert(perm[i] >= 'a' && perm[i] <= 'z');
        value_to_letter[i] = perm[i] - 'a';
    }
    for (i = 0; i < 26; i++) {
        letter_to_value[value_to_letter[i]] = i;
    }
}

static int word_to_int(char *word)
{
    int i;
    int ret = 0;
    for (i = 3; i >= 0; i--)
        ret = (ret * 26) + letter_to_value[word[i] - 'A'];
    return ret;
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

static void word_int_test(void)
{
    int i;
    gen_permutation(letter_to_value, 26);

    for (i = 0; i < NUM_WORDS; i++) {
        char word_again[5];
        int word_val = word_to_int(words[i]);
        int_to_word(word_val, word_again);
        assert(!strcmp(words[i], word_again));
    }
}

// encoded value with variable-nibble format
//
// Bit pattern            Values
// 0xxx:                    1-8
// 1xxx0xxx:                9-72
// 1xxx1xxx0xxx:           73-584
// 1xxx1xxx1xxx0xxx:      585-4680
// 1xxx1xxx1xxx1xxx0xxx: 4681-37449
//
// Big endian, for simpler decoding
static unsigned nibble_encode(int delta, uint8_t *buf)
{
    assert(delta > 0);
    // calculate nibbles (inherently little-endian)
    unsigned nibbles = 0;
    uint8_t out[10];
    while (delta) {
        delta--;
        out[nibbles++] = delta & 7;
        delta >>= 3;
    }
    if (!buf)
        return 4 * nibbles;

    // write nibbles big-endian
    int bits = 0;
    while (nibbles--) {
        int val = out[nibbles];
        if (nibbles)
            val |= 8;
        if (bits & 4)
            *buf++ |= val << 4;
        else
            *buf = val;
        bits += 4;
    }
    return bits;
}

static unsigned nibble_decode(int *delta_out, uint8_t *buf)
{
    int nib = 0;
    int val;
    int delta = 0;
    do {
        if (nib & 1) {
            val = (*buf++) >> 4;
        } else {
            val = *buf & 0xF;
        }
        nib++;
        delta = (delta << 3) | (val & 7);
        delta++;
    } while (val & 0x8);
    *delta_out = delta;
    return nib * 4;
}

static unsigned nibble_encoded_size(int delta)
{
    return nibble_encode(delta, NULL);
}

static void nibble_encode_test(void)
{
    int delta;
    for (delta = 1; delta < 60000; delta++) {
        int delta_dec;
        uint8_t buf[20];
        memset(buf, 0, 20);
        unsigned bits = nibble_encode(delta, NULL);
        assert(bits == nibble_encode(delta, buf));
        assert(bits == nibble_decode(&delta_dec, buf));
        assert(bits == nibble_encoded_size(delta));
    }
    assert(nibble_encode(8, NULL) == 4);
    assert(nibble_encode(1, NULL) == 4);
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
        int delta = word_values[i] - last;
        last = word_values[i];
        bits += nibble_encoded_size(delta);
    }

    unsigned bytes = (bits + 7) / 8 + 26;
    return bytes;
}

static void reseed(void)
{
    FILE *rand = fopen("/dev/urandom", "r");
    fread(&s, sizeof(uint64_t), 2, rand);
    fclose(rand);
}

static char *permutations[] = {
    "nuaioerytmfvjwgdlhscbkzxpq",
    "ueclwpkbyjhsaivdntofxmgrzq",
    NULL
};

int main(void)
{
    int i;
    FILE *wordlist = fopen("list.txt", "r");
    for (i = 0; i < NUM_WORDS; i++)
        fscanf(wordlist, " %4s ", words[i]);
    fclose(wordlist);

    word_int_test();
    nibble_encode_test();

    char **test;
    for (test = permutations; *test != NULL; test++) {
        load_from_value_to_letter(*test);
        printf("%s  bytes: %d\n", *test, compute_efficiency());
    }

    reseed();
    unsigned best = -1;
    long long iter = 0;
    while (1) {
        gen_permutation(letter_to_value, 26);
        unsigned bytes = compute_efficiency();
        iter++;
        if (!(iter & 0xFFFFF)) {
            printf("iters: %lld\n", iter);
            reseed();
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
    return 0;
}
