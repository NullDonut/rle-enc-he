#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TOK_ARRAY_MAX 10000000
typedef struct {
    char c;
    int count;
} RLE_Token;

typedef struct {
    RLE_Token *c;
    int count;
    int capacity;
} DA;

RLE_Token tok_array[TOK_ARRAY_MAX];
int tok_array_count = 0;

void encode_line(DA *d, char *str);

DA da_init(int capacity);
void da_append(DA *d, char c, int c_count);
void da_grow(DA *d, int new_capacity);
void da_destroy(DA *d);
void da_print_full(DA d);

int read_file_input(int argc, char **argv, DA *d) {
    if (argc != 2) {
        printf("Error: Expected usage ./rle <file>\n");
        return 1;
    }
    FILE *fp = fopen(argv[1], "r");
    if (!fp) {
        printf("Error: File not found\n");
        return 1;
    }
    char buf[1024];
    while (fgets(buf, sizeof(buf), fp)) {
        encode_line(d, buf);
    }
    fclose(fp);
    return 0;
}

int main(int argc, char **argv) {
    DA array = da_init(25);
    read_file_input(argc, argv, &array);
    da_destroy(&array);
    return 0;
}

void encode_line(DA *d, char *str) {
    int str_len = strlen(str);
    char c = str[0];
    int count = 0;
    for (int i = 0; i < str_len; i++) {
        if (c != str[i]) {
            da_append(d, c, count);
            // append_tok_array(c, count);
            c = str[i];
            count = 0;
        }
        count += 1;
    }
    da_append(d, c, count);
    // append_tok_array(c, count);
}

DA da_init(int capacity) {
    DA da = {.c = (RLE_Token *)malloc(capacity * sizeof(RLE_Token)),
             .count = 0,
             .capacity = capacity};
    return da;
}
void da_append(DA *d, char c, int c_count) {
    if (d->count == d->capacity) {
        da_grow(d, d->capacity * 2);

        if (d->count == d->capacity)
            return;
    }
    d->c[d->count].c = c;
    d->c[d->count].count = c_count;
    d->count++;
    printf("%d%c (%d/%d)\n", c_count, c, d->count, d->capacity);
}
void da_destroy(DA *d) { free(d->c); }
void da_grow(DA *d, int new_capacity) {
    RLE_Token *temp = realloc(d->c, new_capacity * sizeof(RLE_Token));
    if (!temp) {
        printf("Error: Resizing array failed\n");
        return;
    }
    d->c = temp;
    d->capacity = new_capacity;
}
