#include <stddef.h>
#include <stdint.h>

void * memcpy(void *restrict dest, const void *restrict source, size_t n) {
    uint8_t *db = (uint8_t *)dest;
    const uint8_t *src = (const uint8_t *)source;
    for (size_t i=0; i<n; i++) {
        db[i] = src[i];
    }
    return dest;
}