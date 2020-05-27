#include <stddef.h>
#include <stdint.h>

int memcmp (const void *restrict p1, const void *restrict p2, size_t n) {
    // Short circuit the easy cases
    if (n == 0 || p1 == p2) return 0;

    // use uint8_t (bytes) for the actual comparison
    const uint8_t *b1 = (const uint8_t *) p1;
    const uint8_t *b2 = (const uint8_t *) p2;

    // Do the actual compare
    size_t i=0;
    while (b1[i] == b2[i] && i+1<n) i++;
    return b1[i]-b2[i];
}