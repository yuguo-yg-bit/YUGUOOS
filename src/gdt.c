#include "gdt.h"
#include "serial.h"

struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  access;
    uint8_t  limit_high_flags;
    uint8_t  base_high;
} __attribute__((packed));

struct gdt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

#define GDT_ENTRIES 5
static struct gdt_entry gdt[GDT_ENTRIES];
static struct gdt_ptr   gdt_ptr;

static void gdt_set_entry(int index, uint32_t base, uint32_t limit,
                           uint8_t access, uint8_t flags)
{
    gdt[index].base_low  = base & 0xFFFF;
    gdt[index].base_mid  = (base >> 16) & 0xFF;
    gdt[index].base_high = (base >> 24) & 0xFF;

    gdt[index].limit_low = limit & 0xFFFF;
    gdt[index].limit_high_flags = ((limit >> 16) & 0x0F) | (flags << 4);

    gdt[index].access = access;
}

void gdt_init(void)
{
    gdt_ptr.limit = sizeof(gdt) - 1;
    gdt_ptr.base  = (uint64_t)&gdt;

    gdt_set_entry(0, 0, 0, 0, 0);
    gdt_set_entry(1, 0, 0xFFFFF, 0x9A, 0xA);
    gdt_set_entry(2, 0, 0xFFFFF, 0x92, 0xC);
    gdt_set_entry(3, 0, 0xFFFFF, 0xFA, 0xA);
    gdt_set_entry(4, 0, 0xFFFFF, 0xF2, 0xC);

    asm volatile(
        "lgdt (%0)\n"
        "movq %1, %%rax\n"
        "pushq %%rax\n"
        "leaq 1f(%%rip), %%rax\n"
        "pushq %%rax\n"
        "lretq\n"
        "1:\n"
        "movw %2, %%ax\n"
        "movw %%ax, %%ds\n"
        "movw %%ax, %%es\n"
        "movw %%ax, %%fs\n"
        "movw %%ax, %%gs\n"
        "movw %%ax, %%ss\n"
        :
        : "r"(&gdt_ptr),
          "i"(GDT_KERNEL_CODE),
          "i"(GDT_KERNEL_DATA)
        : "rax", "memory"
    );

    serial_write("[GDT] 内核 GDT 初始化完成\n");
}