ENTRY(_start)

kernel_LMA = 0x00100000;

SECTIONS
{
	. = kernel_LMA;
	kernel_start = .;

	. = ALIGN(4K);
	.text BLOCK(4K) :
	{
		*(.multiboot)
		*(.text)
	}
	
	. = ALIGN(4K);
	.rodata BLOCK(4K) :
	{
		*(.rodata)
	}
	
	. = ALIGN(4K);
	.data BLOCK(4K) :
	{
		*(.data)
	}
	
	. = ALIGN(4K);
	.bss BLOCK(4K) :
	{
		*(COMMON)
		*(.bss)
	}
	
	kernel_end = .;
}