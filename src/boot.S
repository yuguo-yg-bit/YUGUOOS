.code32
.section .multiboot2, "a"
.align 8

multiboot2_header_start:
    .long 0xE85250D6
    .long 0
    .long multiboot2_header_end - multiboot2_header_start
    .long -(0xE85250D6 + 0 + (multiboot2_header_end - multiboot2_header_start))

    .align 8
    .short 5
    .short 1
    .long 20
    .long 1920
    .long 1080
    .long 32

    .align 8
    .short 0
    .short 0
    .long 8
multiboot2_header_end:

.section .boot, "ax"
.globl _start
_start:
    cli

    movl $stack_top, %esp
    movl $stack_top, %ebp

    pushl $0
    popfl

    movl %eax, mb_magic
    movl %ebx, mb_info

    call check_cpuid
    call check_long_mode

    call setup_page_tables
    call enable_paging

    lgdt gdt64_ptr
    ljmpl $0x08, $_start64

    .align 16
check_cpuid:
    pushl %eax
    pushl %ecx
    pushl %edx
    pushfl
    popl %eax
    movl %eax, %ecx
    xorl $0x200000, %eax
    pushl %eax
    popfl
    pushfl
    popl %eax
    pushl %ecx
    popfl
    xorl %ecx, %eax
    jz .no_cpuid
    popl %edx
    popl %ecx
    popl %eax
    movl $1, cpuid_supported
    ret
.no_cpuid:
    movl $0x4F524F45, 0xB8000
    movl $0x4F3A4F52, 0xB8004
    movl $0x4F204F20, 0xB8008
    movl $0x4F554F43, 0xB800C
    movl $0x4F494F50, 0xB8010
    movl $0x4F214F44, 0xB8014
    hlt
    jmp .no_cpuid

check_long_mode:
    movl $0x80000000, %eax
    cpuid
    cmpl $0x80000001, %eax
    jb .no_long_mode

    movl $0x80000001, %eax
    cpuid
    testl $0x20000000, %edx
    jz .no_long_mode
    ret

.no_long_mode:
    movl $0x4F524F45, 0xB8000
    movl $0x4F3A4F52, 0xB8004
    movl $0x4F204F20, 0xB8008
    movl $0x4F664F6C, 0xB800C
    movl $0x4F6F4F6E, 0xB8010
    movl $0x4F214F67, 0xB8014
    hlt
    jmp .no_long_mode

setup_page_tables:
    movl $0x83, %eax
    movl %eax, p3_table
    movl $0, p3_table + 4

    movl $0x40000000, %eax
    orl $0x83, %eax
    movl %eax, p3_table + 8
    movl $0, p3_table + 12

    movl $0x80000000, %eax
    orl $0x83, %eax
    movl %eax, p3_table + 16
    movl $0, p3_table + 20

    movl $0xC0000000, %eax
    orl $0x83, %eax
    movl %eax, p3_table + 24
    movl $0, p3_table + 28

    movl $p3_table, %eax
    orl $0x3, %eax
    movl %eax, p4_table
    movl $0, p4_table + 4
    ret

enable_paging:
    movl $p4_table, %eax
    movl %eax, %cr3

    movl %cr4, %eax
    orl $0xA0, %eax
    movl %eax, %cr4

    movl $0xC0000080, %ecx
    rdmsr
    orl $0x100, %eax
    wrmsr

    movl %cr0, %eax
    orl $0x80000000, %eax
    movl %eax, %cr0
    ret

.code64
_start64:
    cli
    movq $0x10, %rax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss

    movq $stack_top, %rsp
    movq $stack_top, %rbp

    movq $gdt64_ptr, %rax
    lgdt (%rax)

    movl mb_magic, %edi
    movl mb_info, %esi

    call kernel_main

    cli
.hang:
    hlt
    jmp .hang

.section .rodata
.align 16
gdt64:
    .quad 0
    .quad 0x00AF9A000000FFFF
    .quad 0x00AF92000000FFFF
gdt64_ptr:
    .word . - gdt64 - 1
    .quad gdt64

.section .data
.align 4
.globl cpuid_supported
cpuid_supported:
    .long 0
mb_magic:
    .long 0
mb_info:
    .long 0

.section .bss
.align 4096
p4_table:
    .skip 4096
p3_table:
    .skip 4096
p2_table:
    .skip 4096
stack_bottom:
    .skip 16384
stack_top: