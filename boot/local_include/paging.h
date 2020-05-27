#ifndef BOOT_PAGING_H
#define BOOT_PAGING_H

#include <stdint.h>
#include <stdbool.h>

void map_page(uint32_t addr, uint32_t backing, bool isWriteable);
void identity_map_range(uint32_t base, uint32_t limit);
void setup_paging();
void start_paging();

#endif