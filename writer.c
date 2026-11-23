#include "writer.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define MAGIC "RLE1"

// Byte helpers
int put_u32_le(FILE *fp, uint32_t v) {
    unsigned char b[4] = {v, v >> 8, v >> 16, v >> 24};
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

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Error: Expected usage: ./rle <file>\n");
        return EXIT_FAILURE;
    }
    const char *out_path = "enc.rle";
    FILE *in = fopen(argv[1], "rb");
    if (!in) {
        perror(argv[1]);
        return EXIT_FAILURE;
    }
    FILE *out = fopen(out_path, "wb");
    if (!in) {
        perror(out_path);
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
