#include <stdint.h>
#include "local_include/defs.h"

// GDT
gdt_entry gdt[] = {
    {data: {0,0}}, // NULL segment
    {data: {0x0000FFFF, 0x00CF9A00}}, // Kernel code segment
    {data: {0x0000FFFF, 0x00CF9200}}, // Kernel data segment
};

xdt_ptr gdtr = {
    size: sizeof(gdt)-1,
    offset: (uint32_t)(&gdt),
};

//IDT
uint16_t idt[256*4];

xdt_ptr idtr = {
    size: sizeof(idt)-1,
    offset: (uint32_t)(&idt),
};

// Switch over current processor core to using this modules IDT/GDT combo
//  and start processing thread switches. returns when teardown is needed
void switchover() {
    asm volatile (
        "cli\n"
        "lgdt gdtr\n"
        "ljmp $0x08, $1f\n"
        "1:\n"
        "mov $0x08, %%ax\n"
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "lidt idtr\n"
        "int $0x22\n"
        : : : "eax"
    );
}