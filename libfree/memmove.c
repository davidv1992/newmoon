#include <stddef.h>
#include <stdint.h>

void * memmove(void *restrict dest, const void *restrict source, size_t n) {
    // short circuit the no-work case
    if (dest == source) return dest;

    // Because of overlap, we might need to start at the top instead of the bottom
    if (dest < source) {
        uint8_t *db = (uint8_t *)dest;
        const uint8_t *src = (const uint8_t *)source;
        for (size_t i=0; i<n; i++) {
            db[i] = src[i];
        }
    } else {
        uint8_t *db = (uint8_t *)dest;
        const uint8_t *src = (const uint8_t *)source;
        for (size_t i=n; i>0; i--) {
            db[i-1] = src[i-1];
        }
    }

    return dest;
}