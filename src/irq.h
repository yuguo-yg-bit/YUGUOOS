#pragma once
#include <stdint.h>
#include "idt.h"

typedef void (*irq_handler_t)(regs_t *regs);

#define IRQ_TIMER    0
#define IRQ_KEYBOARD 1
#define IRQ_CASCADE  2
#define IRQ_COM2     3
#define IRQ_COM1     4
#define IRQ_LPT2     5
#define IRQ_FLOPPY   6
#define IRQ_LPT1     7
#define IRQ_RTC      8
#define IRQ_ACPI     9
#define IRQ_PCI1     10
#define IRQ_PCI2     11
#define IRQ_MOUSE    12
#define IRQ_FPU      13
#define IRQ_ATA1     14
#define IRQ_ATA2     15

void irq_init(void);
int  irq_register_handler(uint8_t irq, irq_handler_t handler);
int  irq_unregister_handler(uint8_t irq);
void irq_enable(uint8_t irq);
void irq_disable(uint8_t irq);