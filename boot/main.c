#include "../modcore/local_include/defs.h"
#include "../modcore/local_include/transfer_calls.h"
#include "local_include/bootconsole.h"
#include "local_include/multiboot.h"
#include "local_include/pagealloc.h"
#include "local_include/paging.h"
#include "local_include/vm.h"
#include <core.h>

extern int kernel_start, kernel_end;

uint32_t load_module(uint32_t mod_start, uint32_t mod_end);

void transfer(uint32_t addr, uint32_t stack, int id);

uint32_t mod_start;
uint32_t mod_end;

void kernel_main(uint32_t magic, uint32_t mb_infostruct) {
    if (magic != 0x36d76289) {
		// This might not even work depending on what state the loader actually
		// put us in, but hey, might as well try
		bc_print("Invalid bootloader magic %x.\n", magic);
		asm("cli\n\thlt\n\t");
	}
    // Iterate over tags
    mb_info_tag_module *modinfo = 0;

    uint32_t current_tag = mb_infostruct + sizeof(mb_info_fixed);
    mb_info_tag_base *ct = (mb_info_tag_base*)current_tag;
    while (ct->type != 0) {
        bc_print("TAG: %x %x\n", ct->type, ct->size);

        if (ct->type == MB_TAG_MEMMAP) {
            parse_memory_map(current_tag);
        }

        if (ct->type == MB_TAG_MODULE) {
            modinfo = (mb_info_tag_module*)current_tag;
            bc_print("Module from %x to %x\n", modinfo->mod_start, modinfo->mod_end);
        }

        current_tag += ct->size;
        if (ct->size % 8 != 0) current_tag += 8-(ct->size % 8);
        ct = (mb_info_tag_base*)current_tag;
    }

    mod_start = modinfo->mod_start;
    mod_end = modinfo->mod_end;
    mark_alloced(mod_start, mod_end-mod_start);

    setup_paging();
    identity_map_range((uint32_t)&kernel_start, (uint32_t)&kernel_end);
    identity_map_range(0xB8000, 0xB9000);
    identity_map_range(mod_start, mod_end);
    start_paging();

    uint32_t entry = load_module(mod_start, mod_end);
    bc_print("ENTRY: %x\n", entry);
    uint32_t stack = alloc_vm_region(8192);
    map_page(stack, alloc_page(), true);
    map_page(stack+4096, alloc_page(), true);
    transfer(entry, stack+8192, 1);

    while(1);
}

void mod_main() {
    uint32_t testmsg[16];
    listen_interface(0);

    uint32_t entry = load_module(mod_start, mod_end);
    uint32_t stack = alloc_vm_region(8192);
    map_page(stack, alloc_page(), true);
    map_page(stack+4096, alloc_page(), true);
    bc_print("Prepared for second copy %x %x\n", entry, stack);

    create_task((void*)entry, (void*)(stack+8192));

    bc_print("Second copy started\n");

    while(1) {
    recv_message(testmsg, 0);
    bc_print("%d\n", testmsg[0]);
    }
    while(1);
}

extern uint32_t modstack;

stackframe *isr_base_handler(stackframe *frame) {
    if (frame->intno == 0x30) {
        bc_print("%s", (const char*)frame->eax);
        return frame;
    } else if (frame->intno == 0x21 && frame->eax == TRANSFER_INTERFACE) {
        frame->eax = -1;
        return frame;
    } else if (frame->intno == 0x21 && frame->eax == TRANSFER_TASK) {
        frame->eax = 0;
        frame->ebx = TASK_STATE_SELF;
        stackframe *our_stack = (stackframe*)(((uint32_t)&modstack)+4096-sizeof(stackframe));
        our_stack->eip = (uint32_t)mod_main;
        our_stack->cs = 0x08;
        our_stack->ds = 0x10;
        our_stack->es = 0x10;
        our_stack->esp = ((uint32_t)&modstack)+4096;
        asm("pushf\npopl %0\n": "=r"(our_stack->eflags));
        frame->ecx = ((uint32_t)&modstack)+4096-sizeof(stackframe);
        frame->edx = 0;
        return frame;
    }
    bc_print("SOMETHING WENT WRONG\n");
    bc_print("%x %x %x %x\n", frame->intno, frame->error, frame->eip, frame->eax);
    asm("cli\nhlt\n");
    while(1);
    return frame;
}