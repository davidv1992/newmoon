#ifndef KERNEL_MULTIBOOT
#define KERNEL_MULTIBOOT

#include <stdint.h>

#define MB_TAG_NULL    0
#define MB_TAG_CMDLINE 1
#define MB_TAG_LOADER  2
#define MB_TAG_MODULE  3
#define MB_TAG_MEMMAP  6

struct mb_info_fixed {
	uint32_t total_size;
	uint32_t reserved;
} __attribute__((packed));
typedef struct mb_info_fixed mb_info_fixed;

struct mb_info_tag_base {
	uint32_t type;
	uint32_t size;
} __attribute__((packed));
typedef struct mb_info_tag_base mb_info_tag_base;

struct mb_info_tag_cmdline {
	uint32_t type;
	uint32_t size;
	char cmdline[];
} __attribute__((packed));
typedef struct mb_info_tag_cmdline mb_info_tag_cmdline;

struct mb_info_tag_loader {
	uint32_t type;
	uint32_t size;
	char loader_name[];
} __attribute__((packed));
typedef struct mb_info_tag_loader mb_info_tag_loader;

struct mb_info_tag_module {
	uint32_t type;
	uint32_t size;
	uint32_t mod_start;
	uint32_t mod_end;
	char modline[];
} __attribute__((packed));
typedef struct mb_info_tag_module mb_info_tag_module;

struct mb_info_tag_memmap {
	uint32_t type;
	uint32_t size;
	uint32_t entry_size;
	uint32_t entry_version;
} __attribute__((packed));
typedef struct mb_info_tag_memmap mb_info_tag_memmap;

#define MB_MEMMAP_ENTRY_VERSION 0
#define MB_MEMMAP_ENTRY_TYPE_AVAILABLE 1

struct mb_info_memmap_entry{
	uint64_t base_addr;
	uint64_t length;
	uint32_t type;
	uint32_t reserved;
} __attribute__((packed));
typedef struct mb_info_memmap_entry mb_info_memmap_entry;

#endif