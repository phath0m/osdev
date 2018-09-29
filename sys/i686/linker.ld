/* The bootloader will look at this image and start execution at the symbol
   designated as the entry point. */
ENTRY(_start)
 
/* Tell where the various sections of the object files will be put in the final
   kernel image. */
SECTIONS
{
	/* Begin putting sections at 1 MiB, a conventional place for kernels to be
	   loaded at by the bootloader. */
	. = 0xC0100000;
 
	/* First put the multiboot header, as it is required to be put very early
	   early in the image or the bootloader won't recognize the file format.
	   Next we'll put the .text section. */
	.text : AT(ADDR(.text) - 0xC0000000)
	{
		*(.multiboot)
		*(.text)
        *(.init)
        *(.fini)
	}
 
	/* Read-write data (initialized) */
	.data ALIGN (0x1000) : AT(ADDR(.data) - 0xC0000000)
	{
		*(.data)
	}
 
	/* Read-write data (uninitialized) and stack */
	.bss : AT(ADDR(.bss) - 0xC0000000)
	{
        start_bss = .;
		*(COMMON)
		*(.bss)
        end_bss = .;
        *(.bootstrap_stack)
        . += 4096;
        heap_start = .;
	}
 
    .bootstrap_stack : AT(ADDR(.bootstrap_stack) - 0xC0000000)
    {
        *(.bootstrap_stack)
    }
	/* The compiler may produce other sections, by default it will put them in
	   a segment with the same name. Simply add stuff here as needed. */
}
