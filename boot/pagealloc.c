#include "local_include/multiboot.h"
#include "local_include/bootconsole.h"
#include "local_include/pagealloc.h"

#define NENTRIES 32

typedef struct {
    uint32_t start;
    uint32_t length;
} memory_section;

static memory_section free_mem[NENTRIES];
static int nfree = 0;
static memory_section inhib_mem[NENTRIES];
static int ninhib = 0;
static memory_section alloc_mem[NENTRIES];
static int nalloc = 0;

static mb_info_memmap_entry memlist[NENTRIES];
static int nmem = 0;

static uint32_t alloc_base = 0x1000000;
static uint32_t alloc_cur = 0x1000000;
static uint32_t alloc_lim = 0x1000000;

void parse_memory_map(uint32_t memmap_addr) {
    // Determine number of segments in memory map
    mb_info_tag_memmap *memmap_base = (mb_info_tag_memmap*)memmap_addr;
    int entries = (memmap_base->size-sizeof(mb_info_tag_memmap))/memmap_base->entry_size;
    if (entries > NENTRIES) {
        bc_print("Too many segments in memory map");
        asm("cli\nhlt\n");
    }

    // Copy memory map into memlist and sanitize
    for (int i=0; i<entries; i++) {
        mb_info_memmap_entry *entry = (mb_info_memmap_entry*)(memmap_addr + sizeof(mb_info_tag_memmap) + sizeof(mb_info_memmap_entry)*i);

        // Ignore entries laying fully outside the 4GB space
        if (entry->base_addr >= (1LL << 32)) continue;

        // Truncate entries extending above 4GB
        uint64_t base = entry->base_addr;
        uint64_t length = entry->length;
        if (length > (1LL << 32)) length = (1LL << 32);
        if (entry->base_addr + length >= (1LL << 32)) length = (1LL << 32) - entry->base_addr;

        // Round to pages
        base = base & 0xFFFFF000;
        if (length % 4096 != 0) length += 4096 - (length % 4096);

        // Merge into list
        for (int j=0; length != 0 && j < nmem; j++) {
            if (base >= memlist[j].base_addr + memlist[j].length) continue;

            if (base < memlist[j].base_addr) {
                // make space
                if (nmem >= NENTRIES) {
                    bc_print("Too many segments after sanitazation");
                    asm("cli\nhlt\n");
                }
                for (int k=j; k<nmem; k++) {
                    memlist[k+1] = memlist[k];
                }
                nmem++;

                // Fill space
                memlist[j].base_addr = base;
                memlist[j].length = length;
                memlist[j].type = entry->type;
                if (base+length <= memlist[j+1].base_addr) {
                    length = 0;
                    break;
                } else {
                    memlist[j].length = memlist[j+1].base_addr - base;
                    base += memlist[j].length;
                    length -= memlist[j].length;
                }

                j++;
            }

            // Deal with overlap
            if (length != 0) {
                bc_print("Warning: overlapping memory segments");
                if (entry->type == MB_MEMMAP_ENTRY_TYPE_AVAILABLE) {
                    // Ignore overlapping section
                    if (length <= memlist[j].length) length = 0;
                    else length -= memlist[j].length;
                } else {
                    // Take precedence
                    if (length < memlist[j].length) {
                        // make space
                        if (nmem >= NENTRIES) {
                            bc_print("Too many segments after sanitazation");
                            asm("cli\nhlt\n");
                        }
                        for (int k=j; k<nmem; k++) {
                            memlist[k+1] = memlist[k];
                        }
                        nmem++;

                        // Edit next segment
                        memlist[j+1].length -= length;
                        memlist[j+1].base_addr += length;

                        // Fill in current
                        memlist[j].base_addr = base;
                        memlist[j].length = length;
                        memlist[j].type = entry->type;

                        // Mark everything as processed.
                        length = 0;
                    } else {
                        // Mark entire entry with current type
                        memlist[j].type = entry->type;
                        length -= memlist[j].length;
                        base += memlist[j].length;
                    }
                }
            }
        }

        // Add chunk at end
        if (length != 0) {
            if (nmem >= NENTRIES) {
                bc_print("Too many segments after sanitazation");
                asm("cli\nhlt\n");
            }
            memlist[nmem].base_addr = base;
            memlist[nmem].length = length;
            memlist[nmem].type = entry->type;
            nmem++;
        }
    }

    // Process sanitized map
    for (int i=0; i<nmem; i++) {
        // Add entry to appropriate map
        if (memlist[i].type == MB_MEMMAP_ENTRY_TYPE_AVAILABLE) {
            if (nfree >= NENTRIES) {
                bc_print("Run out of free memory segments");
                asm("cli\nhlt\n");
            }
            free_mem[nfree].start = memlist[i].base_addr;
            free_mem[nfree].length = memlist[i].length;
            bc_print("FREE %x %x\n", free_mem[nfree].start, free_mem[nfree].length);
            nfree++;
        } else {
            if (ninhib >= NENTRIES) {
                bc_print("Run out of inhibit memory segments");
                asm("cli\nhlt\n");
            }
            inhib_mem[ninhib].start = memlist[i].base_addr;
            inhib_mem[ninhib].length = memlist[i].length;
            ninhib++;
        }
    }

    // Find allocation limit
    for (int i=0; i<nfree; i++) {
        if (free_mem[i].start <= alloc_base && free_mem[i].start + free_mem[i].length > alloc_base) {
            alloc_lim = free_mem[i].start + free_mem[i].length;
        }
    }
}

void mark_alloced(uint32_t base, uint32_t length) {
    // Round to pages
    base = base & 0xFFFFF000;
    if (length % 4096 != 0) length += 4096 - (length % 4096);

    if (nalloc >= NENTRIES) {
        bc_print("Run out of allocated memory segments");
        asm("cli\nhlt\n");
    }
    alloc_mem[nalloc].start = base;
    alloc_mem[nalloc].length = length;
    nalloc++;

    // Adjust allocation limit
    if (base < alloc_lim && base >= alloc_base) {
        alloc_lim = base;
        if (alloc_lim < alloc_cur) {
            bc_print("Fixed allocated memory overlaps alreayd allocated memory");
            asm("cli\nhlt\n");
        }
    }

    // check for collisions
    if (base < alloc_lim && base + length > alloc_lim) {
        bc_print("Fixed allocated memory overlaps start of boot memory allocation region");
        asm("cli\nhlt\n");
    }
}

uint32_t alloc_page() {
    if (alloc_cur + 4096 > alloc_lim) {
        bc_print("Out of memory during boot");
        asm("cli\nhlt\n");
    }
    uint32_t res = alloc_cur;
    alloc_cur += 4096;
    return res;
}