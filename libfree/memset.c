#include <stddef.h>

void * memset(void *restrict dest, int value, size_t n) {
    unsigned char *buffer = (unsigned char *) dest;
    for (size_t i=0; i<n; i++) buffer[i] = (unsigned char) value;
    return dest;
}