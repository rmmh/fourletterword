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

These numbers do not include the size of the decoder. LZMA has impressive results, but the decoder is larger than the original file size.
