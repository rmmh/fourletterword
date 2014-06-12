#include <assert.h>
#include <error.h>
#include <stdio.h>
#include <stdint.h>

////////////////////////////
// xorshift128+ generator //
////////////////////////////
uint64_t s[ 2 ];

static uint64_t next(void) {
    uint64_t s1 = s[ 0 ];
    const uint64_t s0 = s[ 1 ];
    s[ 0 ] = s0;
    s1 ^= s1 << 23; // a
    return ( s[ 1 ] = ( s1 ^ s0 ^ ( s1 >> 17 ) ^ ( s0 >> 26 ) ) ) + s0; // b, c
}
////////////////////////////

int int_compar(const void *a, const void *b)
{
    return *(int*)a - *(int*)b;
}

#define NUM_WORDS 5526

char words[NUM_WORDS][5];

uint8_t alphabet[26];
uint8_t alphabet_rev[26];

static int word_to_int(char *word)
{
    int ret = 0;
    do {
        ret = (ret * 26) + alphabet[(*word - 'A')];
    } while (*++word);
    return ret;
}

static void build_rev(void)
{
    int i;
    for (i = 0; i < 26; i++) {
        alphabet_rev[alphabet[i]] = i;
    }
}

static int word_to_int_le(char *word)
{
    int i;
    int ret = 0;
    for (i = 3; i >= 0; i--)
        ret = (ret * 26) + alphabet[(word[i] - 'A')];
    return ret;
}

static void int_to_word(int word_int, char *out)
{
    build_rev();
    int i;
    for (i = 3; i >= 0; i--) {
        out[i] = alphabet_rev[word_int % 26] + 'A';
        word_int /= 26;
    }
    out[4] = 0;
}

static void int_to_word_le(int word_int, char *out)
{
    build_rev();
    int i;
    for (i = 0; i < 4; i++) {
        out[i] = alphabet_rev[word_int % 26] + 'A';
        word_int /= 26;
    }
    out[4] = 0;
}

static void shuffle_alphabet(void)
{
    int i, j;

    // initialize & shuffle
    for (i = 0; i < 26; i++) {
        j = next() % (i + 1);
        if (j != i)
            alphabet[i] = alphabet[j];
        alphabet[j] = i;
    }
}

static int bit_size(int delta)
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
    errx(1, "huge valued %d", delta);
}

static int compute_efficiency(void)
{
    int i;
    int word_values[NUM_WORDS];
    for (i = 0; i < NUM_WORDS; i++)
        word_values[i] = word_to_int_le(words[i]);
    qsort(word_values, NUM_WORDS, sizeof(int), int_compar);

    int last = 0;
    int bits = 0;
    for (i = 0; i < NUM_WORDS; i++) {
        int delta = word_values[i] - last - 1;
        last = word_values[i];
        bits += bit_size(delta);
        continue;

        char word[5];
        int_to_word(word_values[i], word);
        printf("%d %d %d %s\n", word_values[i], delta, bit_size(delta), word);
    }

    return bits;
}

int main(int argc, char **argv)
{
    int i;
    FILE *wordlist = fopen("list.txt", "r");
    for (i = 0; i < NUM_WORDS; i++)
        fscanf(wordlist, " %4s ", &words[i]);
    fclose(wordlist);

    FILE *rand = fopen("/dev/urandom", "r");
    fread(&s, sizeof(uint64_t), 2, rand);
    fclose(rand);

    shuffle_alphabet();

    for (i = 0; i < NUM_WORDS; i++) {
        char *word = words[i];
        char word_again[5];
        int word_val = word_to_int_le(words[i]);
        int_to_word_le(word_val, word_again);
        assert(!strcmp(words[i], word_again));
        //printf("%s %d %s\n", word, word_val, word_again);
    }

    int best = 10000;
    long long iter = 0;
    while (1) {
        shuffle_alphabet();
        int bits = compute_efficiency();
        int bytes = (bits + 7) / 8 + 26;
        iter++;
        if (!(iter & 0xFFFFF)) {
            printf("iters:%ld\n", iter);
        }
        if (bytes <= best) {
            best = bytes;
            printf("%4d alphabet: ", bytes);

            build_rev();

            for (i = 0; i < 26; i++) printf("%c", 'a' + alphabet[i]);
            printf("   rev: ");
            for (i = 0; i < 26; i++) printf("%c", 'a' + alphabet_rev[i]);
            printf(" iters:%ld\n", iter);
            iter = 0;
        }
    }
}
