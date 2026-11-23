#include <stdint.h>
#include <stdio.h>

int write_binary(char *path, uint32_t value);
int read_binary(char *path, uint32_t *stream);

int main(void) {
    uint32_t st;

    read_binary("test.bin", &st);
    printf("%du\n", st);
}

int write_binary(char *path, uint32_t value) {
    FILE *fp = fopen(path, "wb");
    fwrite(&value, sizeof(value), 1, fp);
    fclose(fp);
    return 0;
}
int read_binary(char *path, uint32_t *stream) {
    FILE *fp = fopen(path, "rb");
    fread(stream, sizeof(*stream), 1, fp);
    fclose(fp);
    return 0;
}
