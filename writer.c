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
        ;
    return 0;
}
// Test conditions to check byte functions
/**
int main(void) {
    uint32_t tests[] = {0, 1, 255, 256, 0x11223344, 0x80000000u,
0xFFFFFFFFu}; size_t n = sizeof tests / sizeof tests[0]; FILE *f =
fopen("t.bin", "wb"); if (!f) { perror("t.bin"); return EXIT_FAILURE;
    }
    for (int i = 0; i < n; i++) {
        if (put_u32_le(f, tests[i]) != 0) {
            fprintf(stderr, "write failed.\n");
            return EXIT_FAILURE;
        }
    }
    if (fclose(f) == EOF) {
        perror("close");
        return EXIT_FAILURE;
    }

    f = fopen("t.bin", "rb");
    if (!f) {
        perror("t.bin");
        return EXIT_FAILURE;
    }
    for (size_t i = 0; i < n; i++) {
        uint32_t v;
        if (get_u32_le(f, &v) != 0) {
            fprintf(stderr, "read failed at %zu\n", i);
            return EXIT_FAILURE;
        }
        printf("%08x %s\n", v, v == tests[i] ? "ok" : "MISMATCH");
    }
    uint32_t extra;
    printf("read past end:%d (expect -1)\n", get_u32_le(f, &extra));
    fclose(f);
    return 0;
}
**/
