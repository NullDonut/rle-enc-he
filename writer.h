#ifndef WRITER_H
#define WRITER_H

#include <stdint.h>
#include <stdlib.h>

int put_u32_le(FILE *fp, uint32_t v);
int get_u32_le(FILE *fp, uint32_t *v);

#endif
