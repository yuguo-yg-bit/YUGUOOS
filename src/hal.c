#include "hal.h"

uint8_t inb(uint16_t port)
{
    uint8_t value;
    asm volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

uint16_t inw(uint16_t port)
{
    uint16_t value;
    asm volatile("inw %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

uint32_t inl(uint16_t port)
{
    uint32_t value;
    asm volatile("inl %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

void outb(uint16_t port, uint8_t value)
{
    asm volatile("outb %0, %1" :: "a"(value), "Nd"(port));
}

void outw(uint16_t port, uint16_t value)
{
    asm volatile("outw %0, %1" :: "a"(value), "Nd"(port));
}

void outl(uint16_t port, uint32_t value)
{
    asm volatile("outl %0, %1" :: "a"(value), "Nd"(port));
}

void io_wait(void)
{
    outb(0x80, 0);
}

void sti(void)
{
    asm volatile("sti");
}

void cli(void)
{
    asm volatile("cli");
}

void hlt(void)
{
    asm volatile("hlt");
}

void pause_cpu(void)
{
    asm volatile("pause");
}

uint64_t rdmsr(uint32_t msr)
{
    uint32_t low, high;
    asm volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
    return ((uint64_t)high << 32) | low;
}

void wrmsr(uint32_t msr, uint64_t value)
{
    uint32_t low  = (uint32_t)(value & 0xFFFFFFFF);
    uint32_t high = (uint32_t)(value >> 32);
    asm volatile("wrmsr" :: "a"(low), "d"(high), "c"(msr));
}

uint64_t get_cr0(void)
{
    uint64_t value;
    asm volatile("movq %%cr0, %0" : "=r"(value));
    return value;
}

void set_cr0(uint64_t value)
{
    asm volatile("movq %0, %%cr0" :: "r"(value));
}

uint64_t get_cr2(void)
{
    uint64_t value;
    asm volatile("movq %%cr2, %0" : "=r"(value));
    return value;
}

uint64_t get_cr3(void)
{
    uint64_t value;
    asm volatile("movq %%cr3, %0" : "=r"(value));
    return value;
}

void set_cr3(uint64_t value)
{
    asm volatile("movq %0, %%cr3" :: "r"(value) : "memory");
}

void load_gdt(uint64_t gdt_ptr)
{
    asm volatile("lgdt (%0)" :: "r"(gdt_ptr) : "memory");
}

void load_idt(uint64_t idt_ptr)
{
    asm volatile("lidt (%0)" :: "r"(idt_ptr) : "memory");
}

void ltr(uint16_t selector)
{
    asm volatile("ltr %0" :: "r"((uint16_t)selector));
}