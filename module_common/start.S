.section .text

.global _module_entry
_module_entry:
    sub $12, %esp
    pushl %eax
    call module_main
1:
    jmp 1b