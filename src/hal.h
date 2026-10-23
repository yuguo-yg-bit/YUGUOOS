#pragma once
#include <stdint.h>

uint8_t  inb(uint16_t port);
uint16_t inw(uint16_t port);
uint32_t inl(uint16_t port);

void outb(uint16_t port, uint8_t  value);
void outw(uint16_t port, uint16_t value);
void outl(uint16_t port, uint32_t value);

void io_wait(void);

void sti(void);
void cli(void);
void hlt(void);
void pause_cpu(void);

uint64_t rdmsr(uint32_t msr);
void     wrmsr(uint32_t msr, uint64_t value);

uint64_t get_cr0(void);
void     set_cr0(uint64_t value);
uint64_t get_cr2(void);
uint64_t get_cr3(void);
void     set_cr3(uint64_t value);

void load_gdt(uint64_t gdt_ptr);
void load_idt(uint64_t idt_ptr);
void ltr(uint16_t selector);