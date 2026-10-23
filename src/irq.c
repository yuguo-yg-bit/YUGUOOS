#include "irq.h"
#include "idt.h"
#include "pic.h"
#include "serial.h"
#include <stddef.h>

static irq_handler_t irq_handlers[16];

void irq_init(void)
{
    for (int i = 0; i < 16; i++) {
        irq_handlers[i] = NULL;
    }
    serial_write("[IRQ] IRQ 管理层初始化完成\n");
}

int irq_register_handler(uint8_t irq, irq_handler_t handler)
{
    if (irq >= 16) return -1;
    if (irq_handlers[irq] != NULL) return -1;
    irq_handlers[irq] = handler;
    isr_register_handler(32 + irq, handler);
    return 0;
}

int irq_unregister_handler(uint8_t irq)
{
    if (irq >= 16) return -1;
    irq_handlers[irq] = NULL;
    isr_register_handler(32 + irq, NULL);
    return 0;
}

void irq_enable(uint8_t irq)
{
    if (irq >= 16) return;
    pic_unmask(irq);
}

void irq_disable(uint8_t irq)
{
    if (irq >= 16) return;
    pic_mask(irq);
}