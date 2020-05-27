#include "../modloader/local_include/elf.h"
#include "local_include/bootconsole.h"
#include "local_include/vm.h"
#include "local_include/pagealloc.h"
#include "local_include/paging.h"

uint32_t load_module(uint32_t mod_start, uint32_t mod_end) {
    // Validate header is within module
    if (mod_end - mod_start < sizeof(elf_header)) {
        panic("Not enough space for elf header\n");
    }
    elf_header *header = (elf_header*)mod_start;
    if (header->ehsize > mod_end-mod_start) {
        panic("Elf header larger than loaded size\n");
    }

    // Validate header type
    if (header->ident[0] != 0x7f || header->ident[1] != 'E'
        || header->ident[2] != 'L' || header->ident[3] != 'F') {
        panic("Module not elf file\n");
    }
    if (header->type != ELF_TYPE_REL) {
        panic("Module not relocatable\n");
    }
    if (header->ident[4] != ELF_CLASS_32 || header->ident[5] != ELF_DATA2_LSB
        || header->ident[6] != 1) {
        panic("Module architecture not compatible\n");
    }

    // Extract section headers
    if (header->shnum > 32) {
        panic("Too many sections\n");
    }
    if (header->shoff >= mod_end-mod_start || header->shnum*header->shentsize > mod_end-mod_start-header->shoff) {
        panic("Section headers outside of loaded module\n");
    }
    elf_section_header *section_headers[32];
    for (int i=0; i<header->shnum; i++) {
        section_headers[i] = (elf_section_header*)(mod_start + header->shoff + header->shentsize*i);
    }

    // Determine final size and offset to each other of loaded sections
    uint32_t cur_len = 0;
    for (int i=0; i<header->shnum; i++) {
        if (section_headers[i]->flags & ELF_HEADER_FLAG_ALLOC) {
            if (cur_len % section_headers[i]->addralign) {
                cur_len += section_headers[i]->addralign - (cur_len % section_headers[i]->addralign);
            }
            section_headers[i]->addr = cur_len;
            cur_len += section_headers[i]->size;
        }
    }

    // Allocate virtual memory space for binary, and back it with pages
    if (cur_len % 4096) {
        cur_len &= 0xFFFFF000;
        cur_len += 0x00001000;
    }
    uint32_t base = alloc_vm_region(cur_len);
    for(uint32_t fill = 0; fill < cur_len; fill += 4096) {
        uint32_t page = alloc_page();
        map_page(base + fill, page, true);
    }

    // Copy/load all loadable sections
    for (int i=0; i<header->shnum; i++) {
        if (section_headers[i]->flags & ELF_HEADER_FLAG_ALLOC) {
            section_headers[i]->addr += base;
            if (section_headers[i]->type == ELF_HEADER_TYPE_NOBITS) {
                for (uint32_t j=0; j<section_headers[i]->size; j++) {
                    *(uint8_t*)(section_headers[i]->addr + j) = 0;
                }
            } else {
                if (section_headers[i]->offset >= mod_end-mod_start ||
                    section_headers[i]->size >= mod_end-mod_start-section_headers[i]->offset) {
                    panic("Section out of loaded module\n");
                }

                for (uint32_t j=0; j<section_headers[i]->size; j++) {
                    *(uint8_t*)(section_headers[i]->addr + j) = *(uint8_t*)(mod_start + section_headers[i]->offset + j);
                }
            }
        }
    }

    // Relocate all loadable sections
    for (int i=0; i<header->shnum; i++) {
        if (section_headers[i]->type != ELF_HEADER_TYPE_REL) continue;

        // Check related sections
        uint32_t rel_sect = section_headers[i]->info;
        uint32_t sym_sect = section_headers[i]->link;
        if (rel_sect >= header->shnum || sym_sect >= header->shnum) {
            panic("Invalid linked sections\n");
        }

        if (!(section_headers[rel_sect]->flags & ELF_HEADER_FLAG_ALLOC)) continue;
        if (section_headers[i]->offset >= mod_end-mod_start ||
            section_headers[i]->size >= mod_end-mod_start-section_headers[i]->offset) {
                panic("Section out of loaded module\n");
        }
        if (section_headers[sym_sect]->offset >= mod_end-mod_start ||
            section_headers[sym_sect]->size >= mod_end-mod_start-section_headers[sym_sect]->offset){
            panic("Symbol section out of loaded module\n");
        }
        elf_symbol *symtab = (elf_symbol*)(mod_start + section_headers[sym_sect]->offset);
        
        // Iterate over relocations
        for (int j=0; j*sizeof(elf_rel) < section_headers[i]->size; j++) {
            elf_rel *rel = (elf_rel*)(mod_start + section_headers[i]->offset + j*sizeof(elf_rel));

            int type = rel->info & 0xF;
            int symb = rel->info >> 8;
            uint32_t offset = rel->offset;

            if (offset+4 > section_headers[rel_sect]->size) {
                panic("Relocation destination out of bound %x %x\n", offset+4, section_headers[rel_sect]->size);
            }
            if ((symb+1) * sizeof(elf_symbol) > section_headers[sym_sect]->size) {
                panic("Symbol out of bound");
            }

            uint32_t symbolValue = symtab[symb].value;
            if (symtab[symb].shndx != ELF_SECTION_ABS) {
                if (symtab[symb].shndx >= header->shnum) {
                    panic("Symbol section index out of bounds");
                }
                symbolValue += section_headers[symtab[symb].shndx]->addr;
            }

            uint32_t *reloc_dest = (uint32_t*)(section_headers[rel_sect]->addr + offset);
            if (type == ELF_RELOC_32) {
                *reloc_dest += symbolValue;
            } else if (type == ELF_RELOC_PC32) {
                *reloc_dest += symbolValue;
                *reloc_dest -= (section_headers[rel_sect]->addr + offset);
            } else {
                panic("Unknown relocation type %d\n", type);
            }
        }
    }

    return base + header->entry;
}