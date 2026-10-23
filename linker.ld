OUTPUT_FORMAT(elf64-x86-64)
ENTRY(_start)

SECTIONS
{
    . = 2M;

    .boot : ALIGN(4K)
    {
        KEEP(*(.multiboot2))
        *(.boot)
    }

    .text ALIGN(4K) : AT(ADDR(.text))
    {
        *(.text*)
    }

    .rodata ALIGN(4K) : AT(ADDR(.rodata))
    {
        *(.rodata*)
    }

    .data ALIGN(4K) : AT(ADDR(.data))
    {
        *(.data*)
    }

    .bss ALIGN(4K) : AT(ADDR(.bss))
    {
        *(COMMON)
        *(.bss*)
    }

    _kernel_end = .;
}