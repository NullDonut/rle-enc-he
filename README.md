# rle-enc

A small command-line run-length encoder/decoder in C. It works on arbitrary
bytes (text, binary, NULs) and streams its input, so memory use stays flat on
large files.

## Build and test

```
make          # builds ./rle with -Wall -Wextra and ASan/UBSan
make test     # runs test.sh (round trips, boundaries, fuzzing, corrupt files)
```

## Usage

```
./rle <file>            # encode  -> enc.rle
./rle -d <file.rle>     # decode  -> out.bin
```

Errors go to stderr and the exit code is non-zero. A failed encode or decode
removes its partially written output file.

## File format (version 1, magic `RLE1`)

All integers are little-endian `uint32`.

| Offset | Size    | Meaning                                  |
|--------|---------|------------------------------------------|
| 0      | 4       | Magic `RLE1`                             |
| 4      | 4       | Original (decoded) length in bytes       |
| 8...   | 5 each  | Run: 4-byte count, then 1 data byte      |

Example: `aaaabbbccd` encodes to
`52 4c 45 31 | 0a 00 00 00 | 04 00 00 00 61 | 03 00 00 00 62 | 02 00 00 00 63 | 01 00 00 00 64`.

The decoder rejects: a wrong magic, a truncated header or record, a run of
length 0, a run that would exceed the header length, and a total that does not
match the header length.

## Compression

Every run costs 5 bytes regardless of its length, so v1 only saves space when
runs are long (average more than 5 bytes).

| Input                    | Input size | Output size |
|--------------------------|-----------:|------------:|
| 1 MB of zeros            |  1,000,000 |          13 |
| 1,100 bytes of one char  |      1,100 |          13 |
| C source text            |      3,517 |      14,598 |
| 1 MB random bytes        |  1,000,000 |   4,980,808 |

Ordinary text and random data expand by roughly 4-5x. A PackBits-style format
(version 2) is planned to fix this.

## Limits

- Inputs of 4 GiB or more are rejected (the length field is 32-bit).
- Output file names are fixed (`enc.rle`, `out.bin`).
