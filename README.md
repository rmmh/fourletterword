Four-Letter Word
====

The Collins Scrabble Dictionary (CSW12) has 5526 four letter English words.

How small can a program that outputs these words be?

Benchmarks
----

Method | Bytes
------ | -----
raw | 27630
strip newlines | 22104
treat words as base-32, pack at 3 bytes per word | 16578
gzip | 13179
bzip2  | 12970
pack last three letters at 2 bytes per word, index from first letter to suffixes | 11104
lzma | 5569
strip newlines, then lzma | 4737
reverse, sort, strip, lzma | 4540
Encoding A: nibble-encoded posting list deltas | 4019

These numbers do not include the size of the decoder. LZMA has impressive results, but the decoder is larger than the original file size.

Encoding A
----
Convert words to numbers, and encode the gaps between numbers. Improve efficiency by making gaps shorter overall.

Words can be converted to integers by treating each letter as a value, 0-25, and doing a base-26 conversion. For example, 'WORD' = (22, 14, 17, 3) = 22*26^0 + 14*26^1 + 17*26^2 + 3*26^3 = 64606.

The list of numbers can be sorted and compressed by encoding the difference (delta) from the previous number.

To encode a difference, a variable length nibble-based coding is used. The high bit of each nibble indicates if it is the last one.

    Bit pattern            Values
	0xxx                     1-8
	1xxx0xxx                 9-72
	1xxx1xxx0xxx            73-584
	1xxx1xxx1xxx0xxx       585-4680
	1xxx1xxx1xxx1xxx0xxx  4681-37449

This method of encoding deltas is similar to those used in search engines for compressing indexes or posting lists, where each word has a list of documents it occurs in represented by a sequence of integers.

Compression efficiency is improved by modifying the base conversion. Treating the first letter as the least significant digit (the ones place) saves ~200B, since four letter words share many suffixes. For example, 113 words end in 'ES', so the gaps between 'DIES', 'PIES', and 'TIES' might be small enough to be encoded in only 3 nibbles.

Changing the numeric value associated with each letter yields large gains as well. The alphabet is not optimal for minimizing gaps between words. There are 26! = 4*10^26 ways to map letters to values, which is too large to search exhaustively. Picking points at random is unlikely to find a solution close to optimal. Instead, a simple stochastic optimization called Random Search is used. A permutations is randomly selected and repeatedly changed slightly to attempt to find a permutation with a smaller encoding. With enough search time, the best local minimum found should be close to the global minimum.
