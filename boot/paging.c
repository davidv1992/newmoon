#include <stdbool.h>
#include <stdint.h>
#include "local_include/pagealloc.h"

#define CUR_REC_MAPPING 1023

#define DIR_BITS    0xFFC00000
#define DIR_SHIFT   22
#define PAGE_BITS   0x003FF000
#define PAGE_SHIFT  12
#define INPAGE_BITS 0x00000FFF
#define ENTRY_SHIFT 2

#define PAGE_PRESENT   0x1
#define PAGE_WRITEABLE 0x2
#define FLAG_MASK      (PAGE_WRITEABLE | PAGE_USERSPACE)

static uint32_t *pd;
static bool active;

static inline uint32_t* get_pd_entry_rec(uint32_t dir_entry) {
	return (uint32_t*)((CUR_REC_MAPPING << DIR_SHIFT) | (CUR_REC_MAPPING << PAGE_SHIFT) | (dir_entry << ENTRY_SHIFT));
}

static inline uint32_t* get_pt_entry_rec(uint32_t dir_entry, uint32_t page_entry) {
	return (uint32_t*)((CUR_REC_MAPPING << DIR_SHIFT) | (dir_entry << PAGE_SHIFT) | (page_entry << ENTRY_SHIFT));
}

static inline uint32_t* get_pd_entry_lin(uint32_t dir_entry) {
    return &pd[dir_entry];
}

static inline uint32_t* get_pt_entry_lin(uint32_t dir_entry, uint32_t page_entry) {
    return &((uint32_t*)(*get_pd_entry_lin(dir_entry) & ~INPAGE_BITS))[page_entry];
}

static inline uint32_t make_table_entry(uint32_t addr, bool isWriteable) {
    uint32_t flags = PAGE_PRESENT;
    if (isWriteable) flags |= PAGE_WRITEABLE;

    return (addr & ~INPAGE_BITS) | flags;
}

void map_page(uint32_t addr, uint32_t backing, bool isWriteable) {
    uint32_t dir_entry = (addr & DIR_BITS) >> DIR_SHIFT;
	uint32_t page_entry = (addr & PAGE_BITS) >> PAGE_SHIFT;

    if (!active) {
        if (!(*get_pd_entry_lin(dir_entry) & PAGE_PRESENT)) {
            uint32_t newdir = alloc_page();
            *get_pd_entry_lin(dir_entry) = make_table_entry(newdir, true);
            for (int i=0; i<1024; i++)
			    *get_pt_entry_lin(dir_entry, i) = 0;
        }
        *get_pt_entry_lin(dir_entry, page_entry) = make_table_entry(backing, isWriteable);
    } else {
        if (!(*get_pd_entry_rec(dir_entry) & PAGE_PRESENT)) {
            uint32_t newdir = alloc_page();
            *get_pd_entry_rec(dir_entry) = make_table_entry(newdir, true);
            asm volatile ("invlpg (%0)\n\t" : : "r"(get_pt_entry_rec(dir_entry, 0)) : "memory");
            for (int i=0; i<1024; i++)
			    *get_pt_entry_rec(dir_entry, i) = 0;
        }
        *get_pt_entry_rec(dir_entry, page_entry) = make_table_entry(backing, isWriteable);
        asm volatile ("invlpg (%0)\n\t" : : "r"(addr) : "memory");
    }
}

void identity_map_range(uint32_t base, uint32_t limit) {
    base = base & ~INPAGE_BITS;
    if (limit & INPAGE_BITS) {
        limit = (limit & ~INPAGE_BITS) + 4096;
    }

    while (base < limit) {
        map_page(base, base, true);
        base += 4096;
    }
}

void setup_paging() {
    pd = (uint32_t*)alloc_page();
    for (int i=0; i<1024; i++) pd[i] = 0;
    pd[CUR_REC_MAPPING] = make_table_entry((uint32_t)pd, true);
    active = false;
}

void start_paging() {
    int tmp;
    asm volatile (
        "mov %1, %%cr3\n"
        "mov %%cr0, %0\n"
        "or $0x80000000, %1\n"
        "mov %0, %%cr0\n" : "=r" (tmp) : "r" (pd) : "memory"
    );

    active = true;
}