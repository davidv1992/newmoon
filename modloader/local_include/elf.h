#ifndef MODLOADER_ELF_H
#define MODLOADER_ELF_H

#include <stdint.h>

typedef struct {
    uint8_t ident[16]; //0x0
    uint16_t type;     //0x10
    uint16_t machine;  //0x12
    uint32_t version;  //0x14
    uint32_t entry;    //0x18
    uint32_t phoff;    //0x1c
    uint32_t shoff;    //0x20
    uint32_t flags;    //0x24
    uint16_t ehsize;   //0x28
    uint16_t phentsize; //0x2a
    uint16_t phnum;     //0x2c
    uint16_t shentsize; //0x2e
    uint16_t shnum;     //0x30
    uint16_t shstrndx;  //0x32
} __attribute__((packed)) elf_header;

#define ELF_CLASS_32 1
#define ELF_DATA2_LSB 1
#define ELF_VERSION_CURRENT 1
#define ELF_TYPE_REL 1
#define ELF_MACHINE_386 3

typedef struct {
    uint32_t name;
    uint32_t type;
    uint32_t flags;
    uint32_t addr;
    uint32_t offset;
    uint32_t size;
    uint32_t link;
    uint32_t info;
    uint32_t addralign;
    uint32_t entsize;
} __attribute__((packed)) elf_section_header;

#define ELF_HEADER_TYPE_NULL     0
#define ELF_HEADER_TYPE_PROGBITS 1
#define ELF_HEADER_TYPE_SYMTAB   2
#define ELF_HEADER_TYPE_STRTAB   3
#define ELF_HEADER_TYPE_RELA     4
#define ELF_HEADER_TYPE_HASH     5
#define ELF_HEADER_TYPE_DYNAMIC  6
#define ELF_HEADER_TYPE_NOTE     7
#define ELF_HEADER_TYPE_NOBITS   8
#define ELF_HEADER_TYPE_REL      9
#define ELF_HEADER_TYPE_SHLIB   10
#define ELF_HEADER_TYPE_DYNSYM  11

#define ELF_HEADER_FLAG_ALLOC 2

typedef struct {
    uint32_t offset;
    uint32_t info;
} __attribute__((packed)) elf_rel;

#define ELF_RELOC_32   1
#define ELF_RELOC_PC32 2

typedef struct {
    uint32_t name;
    uint32_t value;
    uint32_t size;
    uint8_t info;
    uint8_t other;
    uint16_t shndx;
} __attribute__((packed)) elf_symbol;

#define ELF_SECTION_UNDEF 0
#define ELF_SECTION_ABS 0xfff1
#define ELF_SECTION_COMMON 0xfff2

#endif