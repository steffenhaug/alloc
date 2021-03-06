#ifndef ALLOC_H
#define ALLOC_H

#include <stddef.h>

void *alloc(size_t n_bytes);
void free(void *ptr);
void hexdump();

#endif
