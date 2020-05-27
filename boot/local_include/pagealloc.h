#ifndef BOOT_PAGEALLOC_H
#define BOOT_PAGEALLOC_H

#include <stdint.h>

void parse_memory_map(uint32_t memmap_addr);
void mark_alloced(uint32_t base, uint32_t length);
uint32_t alloc_page();

#endif