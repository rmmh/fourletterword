#include <assert.h>
#include <error.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "decode.h"

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

static void build_rev(uint8_t *l2v, uint8_t *v2l)
{
    int i;
    for (i = 0; i < 26; i++) {
        v2l[l2v[i]] = i + 'a';
    }
}

static void load_from_value_to_letter(const char *perm)
{
    int i;
    assert(strlen(perm) == 26);
    for (i = 0; i < 26; i++) {
        assert(perm[i] >= 'a' && perm[i] <= 'z');
        value_to_letter[i] = perm[i];
    }
    for (i = 0; i < 26; i++) {
        letter_to_value[value_to_letter[i] - 'a'] = i;
    }
}

static void print_letter_to_value(uint8_t *l2v)
{
    int i;
    uint8_t value_to_letter[26];
    build_rev(l2v, value_to_letter);
    for (i = 0; i < 26; i++)
        printf("%c", value_to_letter[i]);
}

static uint32_t word_to_u32(char *word)
{
    int i;
    unsigned ret = 0;
    for (i = 3; i >= 0; i--)
        ret = (ret * 26) + letter_to_value[word[i] - 'A'];
    return ret;
}

static void word_int_test(void)
{
    int i;
    gen_permutation(letter_to_value, 26);
    build_rev(letter_to_value, value_to_letter);

    for (i = 0; i < NUM_WORDS; i++) {
        char word_again[5];
        int word_val = word_to_u32(words[i]);
        u32_to_word(word_val, word_again);
        assert(!strncasecmp(words[i], word_again, 4));
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
static uint32_t nibble_encode(int delta, uint32_t idx, uint8_t *buf)
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
        return nibbles;

    // write nibbles big-endian
    while (nibbles--) {
        int val = out[nibbles];
        if (nibbles)
            val |= 8;
        if (idx & 1)
            buf[idx>>1] |= val << 4;
        else
            buf[idx>>1] = val;
        idx++;
    }
    return idx;
}


static unsigned nibble_encoded_size(int delta)
{
    return nibble_encode(delta, 0, NULL);
}

static void nibble_encode_test(void)
{
    uint32_t delta;
    for (delta = 1; delta < 60000; delta++) {
        uint32_t delta_dec;
        uint8_t buf[20];

        memset(buf, 0, 20);
        unsigned nib_count = nibble_encode(delta, 0, NULL);
        assert(nib_count == nibble_encoded_size(delta));
        assert(nib_count == nibble_encode(delta, 0, buf));
        assert(nib_count == nibble_decode(&delta_dec, 0, buf));
        assert(delta_dec == delta);

        memset(buf, 0, 20);
        assert(nib_count + 1 == nibble_encode(delta, 1, buf));
        assert(nib_count + 1 == nibble_decode(&delta_dec, 1, buf));
        assert(delta_dec == delta);
    }
    assert(nibble_encode(8, 0, NULL) == 1);
    assert(nibble_encode(1, 0, NULL) == 1);
}

static uint32_t encode(uint8_t *buf)
{
    /* convert words to ints */
    int i;
    int word_values[NUM_WORDS];
    for (i = 0; i < NUM_WORDS; i++)
        word_values[i] = word_to_u32(words[i]);
    qsort(word_values, NUM_WORDS, sizeof(int), int_compar);

    /* compress list of strictly increasing integers by
     * encoding difference from previous word into nibbles*/
    int last = 0;
    uint32_t idx = 0;
    for (i = 0; i < NUM_WORDS; i++)
    {
        int delta = word_values[i] - last;
        last = word_values[i];
        idx = nibble_encode(delta, idx, buf);
    }

    return idx;
}

// determine how many bits it would take to encode the words
// with the current letter_to_value
static unsigned compute_efficiency(void)
{
    uint8_t nibbles_buf[NUM_WORDS * 8];
    uint32_t nibbles = encode(nibbles_buf);
    unsigned bytes = (nibbles + 1) / 2 + 26;
    return bytes;
}

static void minor_change(void)
{
    int a = xrand() % 26;
    int b = xrand() % 26;
    if (a == b)
        return;
    int temp = letter_to_value[a];
    letter_to_value[a] = letter_to_value[b];
    letter_to_value[b] = temp;
}

static void reseed(void)
{
    FILE *rand = fopen("/dev/urandom", "r");
    fread(&s, sizeof(uint64_t), 2, rand);
    fclose(rand);
}

static void emit(char *fname)
{
    unsigned i;
    uint8_t nibbles_buf[NUM_WORDS * 8];
    uint32_t nibbles = encode(nibbles_buf);

    FILE *fout = fopen(fname, "w");
    fprintf(fout, "DATA uint8_t value_to_letter[26] = \"");
    for (i = 0; i < 26; i++)
        fprintf(fout, "%c", value_to_letter[i]);
    fprintf(fout, "\";\n\n");

    fprintf(fout, "DATA uint8_t words_encoded[] = {");

    for (i = 0; i <= nibbles / 2; i++) {
        if (!(i & 0xF))
            fprintf(fout, "\n   ");
        fprintf(fout, " 0x%02X,", nibbles_buf[i]);
    }
    fprintf(fout, "\n};\n");
    fclose(fout);
}

static char *permutations[] = {
    "mgxjqzvbdctwhfkspnlreoaiuy", // 4020
    "nuaioerytmfvjwgdlhscbkzxpq",
    "ueclwpkbyjhsaivdntofxmgrzq",
    "hctpxjvbskzfdgqmwlreoauiny",
    "yoauiernhlpxvkmgdfbzsjqwtc",
    "yuieaorhnlsptkcwgdbvxzjqfm", // 4022
    NULL
};



int main(int argc, char **argv)
{
    int i;
    FILE *wordlist = fopen("list.txt", "r");
    for (i = 0; i < NUM_WORDS; i++)
        fscanf(wordlist, " %4s ", words[i]);
    fclose(wordlist);

    word_int_test();
    nibble_encode_test();

    if (argc >= 2 && !strcmp(argv[1], "-g")) {
        char *fname = "list.gen.h";
        printf("writing data for permutation %s to %s\n", *permutations, fname);
        load_from_value_to_letter(*permutations);
        emit(fname);
        return 0;
    }

    char **test;
    for (test = permutations; *test != NULL; test++) {
        load_from_value_to_letter(*test);
        printf("%s  bytes: %d\n", *test, compute_efficiency());
    }

    reseed();
    unsigned best = -1;
    long long iter = 0;

    /* Attempt to find a good alphabet permutation with Random Search.
     *
     * Algorithm:
     *   Pick a configuration at random from the search space.
     *   Until the maximum number of iterations are performed:
     *     Pick a new configuration close to the best one found
     *     If the new configuration is better, set best to it
     */
    uint8_t global_best[26];
    unsigned best_global = -1;
    while (1) {
        /* initial attempt */
        reseed();
        gen_permutation(letter_to_value, 26);
        best = -1;
        uint8_t iter_best[26];
        for (iter = 0; iter < 50000; iter++) {
            /* try a change */
            minor_change();
            unsigned bytes = compute_efficiency();
            if (bytes <= best) {
                if (bytes < best) {
                    best = bytes;
                    print_letter_to_value(letter_to_value);
                    printf("  bytes: %d  iters: %lld\n", bytes, iter);
                    if (bytes < best_global) {
                        memcpy(global_best, letter_to_value, sizeof(global_best));
                        best_global = bytes;
                    }
                    iter = 0;
                }
                /* keep favorable change */
                memcpy(iter_best, letter_to_value, sizeof(iter_best));
            } else {
                /* revert unfavorable change */
                memcpy(letter_to_value, iter_best, sizeof(iter_best));
            }
        }
        printf("\n\nglobal best: %d ", best_global);
        print_letter_to_value(global_best);
        printf("\n");
    }
    return 0;
}
