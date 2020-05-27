#include <stdint.h>

static uint32_t alloc_cur = 0xC0000000;

uint32_t alloc_vm_region(uint32_t length) {
    uint32_t res = alloc_cur;
    alloc_cur += length;
    return res;
}