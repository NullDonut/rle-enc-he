#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char c;
    int count;
} RLE_Token;

typedef struct {
    RLE_Token *c;
    int count;
    int capacity;
} DA;

typedef struct {
    char current_char;
    int count;
    int has_run;
} EncoderState;

void encode_line(DA *d, EncoderState *state, char *str);

DA da_init(int capacity);
int da_append(DA *d, char c, int c_count);
int da_grow(DA *d, int new_capacity);
void da_destroy(DA *d);

void write_rle_file(DA *d) {
    char *file_path = "enc.rle";
    FILE *fp = fopen(file_path, "w");
    if (!fp) {
        fprintf(stderr, "Error: Opening/Writing to encoded file : %s.",
                file_path);
        exit(EXIT_FAILURE);
    }
    char buf[1024];
    for (int i = 0; i < d->count; i++) {
        snprintf(buf, sizeof(buf), "%d%c", (d->c[i]).count, (d->c)[i].c);
        if (fputs(buf, fp) == EOF) {
            fprintf(stderr,
                    "Error: Failed writing a line due to End of file Error.");
            fclose(fp);
            return;
        }
    }
    if (fclose(fp) == EOF) {
        fprintf(stderr, "Error: Failed to close encoded file.\n");
    }
}
int read_file_input(int argc, char **argv, DA *d) {

    FILE *fp = fopen(argv[1], "r");
    if (!fp) {
        fprintf(stderr, "Error: File not found\n");
        return 1;
    }
    char buf[1024];
    EncoderState state = {0};
    while (fgets(buf, sizeof(buf), fp)) {
        encode_line(d, &state, buf);
    }
    if (state.has_run) {
        da_append(d, state.current_char, state.count);
    }
    fclose(fp);
    return 0;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Error: Expected usage: ./rle <file>\n");
        return EXIT_FAILURE;
    }
    DA array = da_init(25);

    if (read_file_input(argc, argv, &array) == 1) {
        fprintf(stderr, "Error: Some error occured in reading the file.\n");
        da_destroy(&array);
        exit(EXIT_FAILURE);
    }
    write_rle_file(&array);
    da_destroy(&array);
    return 0;
}

void encode_line(DA *d, EncoderState *state, char *str) {
    int str_len = strlen(str);
    for (int i = 0; i < str_len; i++) {
        if (!state->has_run) {
            state->current_char = str[i];
            state->count = 1;
            state->has_run = 1;
        } else if (str[i] == state->current_char) {
            state->count++;
        } else {
            da_append(d, state->current_char, state->count);
            state->current_char = str[i];
            state->count = 1;
        }
    }
}

DA da_init(int capacity) {
    DA da = {.c = (RLE_Token *)malloc(capacity * sizeof(RLE_Token)),
             .count = 0,
             .capacity = capacity};
    if (!(da.c)) {
        fprintf(stderr, "Error: Failed to allocate %d to array.\n", capacity);
        exit(1);
    }
    return da;
}
int da_append(DA *d, char c, int c_count) {
    if (d->count == d->capacity) {
        if (da_grow(d, d->capacity * 2) != 0) {
            fprintf(stderr, "Error: Resizing Dynamic array failed.\n");
            return -1;
        }
    }
    d->c[d->count].c = c;
    d->c[d->count].count = c_count;
    d->count++;
    return 0;
}
void da_destroy(DA *d) { free(d->c); }
int da_grow(DA *d, int new_capacity) {
    RLE_Token *temp = realloc(d->c, new_capacity * sizeof(RLE_Token));
    if (!temp) {
        fprintf(stderr, "Error: Resizing array failed\n");
        return 1;
    }
    d->c = temp;
    d->capacity = new_capacity;
    return 0;
}
