#multiboot header constants
.set MAGIC,            0xE85250D6
.set ARCH,             0
.set TAG_END,          0
.set TAG_MODULE_ALIGN, 6
.set kernel_VMA,       0xC0000000

#multiboot header
.section .multiboot
mb_start:
.align 16
#primary header
.long MAGIC
.long ARCH
.long (mb_end-mb_start)
.long -(MAGIC+ARCH+mb_end-mb_start)
#tags
.hword TAG_MODULE_ALIGN
.hword 1 #Don't ignore
.hword 8
.hword TAG_END
.hword 0
.hword 8
mb_end:

# Boot stack
.section .bss
.align 16
stack_bottom:
.skip 4096 # 4KiB
stack_top:
.global stack_bottom

# Module stack
.section .bss
.align 16
modstack:
.skip 4096
.global modstack

# Segment descriptor table
.section .data
.align 16
#Null descriptor
gdt:
.long 0
.long 0
#Ring 0 Code segment
.long 0x0000FFFF
.long 0x00CF9A00
#Ring 0 Data segment
.long 0x0000FFFF
.long 0x00CF9200

#padding
.hword 0
#GDTR load data
gdtr:
.hword (8*6-1)
.long gdt

#(Emtpy) IDT
.section .data
.global idt
idt:
.skip (256*8) #256 8 byte entries

#padding
.hword 0
#IDTR load data
idtr:
.hword (8*256-1)
.long idt

# Kernel entry
.section .text
.global _start
_start:
	# Initialize a stack
	mov $stack_top, %esp

	# Switch to own GDT and segments
	lgdt gdtr
	mov $0x10, %cx
	mov %cx, %ds
	mov %cx, %ss
	mov $0, %cx
	mov %cx, %es
	mov %cx, %fs
	mov %cx, %gs
	ljmp $0x08, $_initcs
_initcs:
	
	# Init known good flags state
	cld
	
	# Initialize idt table (preserving eax,ebx)
	mov %ebx, %esi
	mov %eax, %edi
	call idtbuildtable
	
	# Load IDT
	lidt idtr
	
	# Call kernel main
	sub $8, %esp
	push %esi
	push %edi
	call kernel_main
	
	#safety in case kernel main ever returns (shouldn't happen)
	cli
1:	hlt
	jmp 1b

# Utility for control transfers
.global transfer
transfer:
	mov 4(%esp), %ebx
	mov 12(%esp), %eax
	mov 8(%esp), %esp
	call *%ebx
	cli
1:	hlt
	jmp 1b