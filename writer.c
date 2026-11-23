#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAGIC "RLE1"

// Byte helpers
int put_u32_le(FILE *fp, uint32_t v) {
    unsigned char b[4] = {(unsigned char)v, (unsigned char)(v >> 8),
                          (unsigned char)(v >> 16), (unsigned char)(v >> 24)};
    return fwrite(b, 1, 4, fp) == 4 ? 0 : -1;
}
int get_u32_le(FILE *fp, uint32_t *v) {
    unsigned char b[4];
    if (fread(b, 1, 4, fp) != 4)
        return -1;
    *v = (uint32_t)b[0] | (uint32_t)b[1] << 8 | (uint32_t)b[2] << 16 |
         (uint32_t)b[3] << 24;
    return 0;
}

// Encoder
int emit_run(FILE *out, unsigned char c, uint32_t n) {
    if (put_u32_le(out, n) != 0)
        return -1;
    return fputc(c, out) == EOF ? -1 : 0;
}

// Encode File using encoder :)
// Lol btw if you reading this code i am adding comments for my readiblity not
// ai.

int encode_file(FILE *in, FILE *out, uint32_t *total) {
    unsigned char buf[4096];
    size_t n;
    int has_run = 0;
    unsigned char cur = 0;
    uint32_t run = 0;
    *total = 0;
    while ((n = fread(buf, 1, sizeof buf, in)) > 0) {
        for (size_t i = 0; i < n; i++) {
            if (*total == UINT32_MAX)
                return -2;
            (*total)++;
            if (has_run && buf[i] == cur && run < UINT32_MAX) {
                run++;
            } else {
                if (has_run && emit_run(out, cur, run) != 0)
                    return -1;
                cur = buf[i];
                run = 1;
                has_run = 1;
            }
        }
    }
    if (ferror(in))
        return -3;
    if (has_run && emit_run(out, cur, run) != 0)
        return -1;
    return 0;
}
int decode_file(FILE *in, FILE *out) {
    char magic[4];
    uint32_t expected, written = 0;
    if (fread(magic, 1, 4, in) != 4 || memcmp(magic, MAGIC, 4) != 0) {
        return -1; // Bad magic
        fprintf(stderr, "Error: not a RLE1 file (bad magic) \n");
        return -1;
    }
    if (get_u32_le(in, &expected) != 0) {
        fprintf(stderr, "Error: truncated header \n");
        return -1;
    }
    for (;;) {
        unsigned char b[5];
        size_t n = fread(b, 1, 5, in);
        if (n == 0)
            break;
        if (n != 5) {
            fprintf(stderr, "Error: truncated record\n");
            return -1;
        }
        uint32_t count = (uint32_t)b[0] | (uint32_t)b[1] << 8 |
                         (uint32_t)b[2] << 16 | (uint32_t)b[3] << 24;
        if (count == 0) {
            fprintf(stderr, "Error: zero-length run \n");
            return -1;
        }
        if (count > expected - written) {
            fprintf(stderr, "Error: data exceeds header length\n");
            return -1;
        }
        for (uint32_t i = 0; i < count; i++) {
            if (fputc(b[4], out) == EOF) {
                fprintf(stderr, "Error: write failed\n");
                return -1;
            }
        }
        written += count;
    }
    if (ferror(in)) {
        fprintf(stderr, "Error: read failed\n");
        return -1;
    }
    if (written != expected) {
        fprintf(stderr, "Error: length mismatch (header %u , data %u)\n",
                expected, written);
        return -1;
    }
    return 0;
}
int main(int argc, char **argv) {
    if (argc == 3 && strcmp(argv[1], "-d") == 0) {
        const char *dec_out = "out.bin";
        FILE *din = fopen(argv[2], "rb");
        if (!din) {
            perror(argv[2]);
            return EXIT_FAILURE;
        }
        FILE *dout = fopen(dec_out, "wb");
        if (!dout) {
            perror(dec_out);
            fclose(din);
            return EXIT_FAILURE;
        }
        int drc = decode_file(din, dout);
        fclose(din);
        if (fclose(dout) == EOF)
            drc = -1;
        if (drc != 0) {
            remove(dec_out);
            return EXIT_FAILURE;
        }
        return 0;
    }
    if (argc != 2) {
        fprintf(stderr,
                "Error: Expected usage: ./rle <file> | ./rle -d <rle_file> \n");
        return EXIT_FAILURE;
    }
    const char *out_path = "enc.rle";
    FILE *in = fopen(argv[1], "rb");
    if (!in) {
        perror(argv[1]);
        return EXIT_FAILURE;
    }
    FILE *out = fopen(out_path, "wb");
    if (!out) {
        perror(out_path);
        fclose(in);
        return EXIT_FAILURE;
    }

    uint32_t total = 0;
    int rc = -1;
    if (fwrite(MAGIC, 1, 4, out) == 4 && put_u32_le(out, 0) == 0)
        rc = encode_file(in, out, &total);
    fclose(in);

    if (rc == 0) {
        if (fseek(out, 4, SEEK_SET) != 0 || put_u32_le(out, total) != 0)
            rc = -1;
    }
    if (fclose(out) == EOF)
        rc = -1;
    if (rc != 0) {
        fprintf(stderr, "Error: encoding failed (code %d)\n", rc);
        remove(out_path);
        return EXIT_FAILURE;
    }
    return 0;
}
