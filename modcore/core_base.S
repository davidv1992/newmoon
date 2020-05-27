.section .text

.global _isr_common
_isr_common:
    # Build up stack frame
    pusha
    cld
    pushl %es
    pushl %ds

    # Reset segment registers
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es

    # Setup work stack
    mov %esp, %eax
    mov $stack_top, %esp
    pushl %eax

    # Handle
    call isr_base_handler

    # Move next task into place
    mov %eax, %esp
    pop %ds
    pop %es
    popa
    add $8, %esp

    # And return
    iret

.global idle_task
idle_task:
    hlt
    jmp idle_task

.section .data

.global stack
.global stack_top

stack:
    .skip 4096
stack_top: