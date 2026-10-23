#include "idt.h"
#include "hal.h"
#include "pic.h"
#include "serial.h"
#include "vga.h"

struct idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  ist;
    uint8_t  type_attr;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t reserved;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

static struct idt_entry idt[256];
static struct idt_ptr   idt_ptr;
static isr_handler_t    isr_handlers[256];

static const char *exception_names[] = {
    "除零错误",
    "调试异常",
    "NMI 不可屏蔽中断",
    "断点异常",
    "溢出异常",
    "越界异常",
    "无效操作码",
    "设备不可用",
    "双重错误",
    "协处理器段超限",
    "无效 TSS",
    "段不存在",
    "栈段错误",
    "通用保护错误",
    "页错误",
    "保留",
    "x87 浮点异常",
    "对齐检查",
    "机器检查",
    "SIMD 浮点异常",
    "虚拟化异常",
    "控制保护异常",
    "保留",
    "保留",
    "保留",
    "保留",
    "保留",
    "保留",
    "超级调用异常",
    "VMM 通信异常",
    "安全异常",
    "保留",
};

static void idt_set_entry(int vector, uint64_t handler, uint16_t selector,
                           uint8_t ist, uint8_t type_attr)
{
    idt[vector].offset_low  = handler & 0xFFFF;
    idt[vector].offset_mid  = (handler >> 16) & 0xFFFF;
    idt[vector].offset_high = (handler >> 32) & 0xFFFFFFFF;
    idt[vector].selector    = selector;
    idt[vector].ist         = ist;
    idt[vector].type_attr   = type_attr;
    idt[vector].reserved    = 0;
}

void isr_handler(regs_t *regs)
{
    if (isr_handlers[regs->int_no] != 0) {
        isr_handlers[regs->int_no](regs);
        return;
    }

    if (regs->int_no < 32) {
        serial_write("\n[异常] ");
        serial_write(exception_names[regs->int_no]);
        serial_write(" (向量 ");
        serial_write_hex(regs->int_no);
        serial_write(")\n");
        serial_write("  RIP=");
        serial_write_hex(regs->rip);
        serial_write(" 错误码=");
        serial_write_hex(regs->err_code);
        serial_write("\n");

        vga_set_color(VGA_LIGHT_RED, VGA_BLACK);
        vga_write("\n[内核异常] ");
        vga_write(exception_names[regs->int_no]);
        vga_write(" @ RIP=");
        vga_write_hex(regs->rip);
        vga_write(" err=");
        vga_write_hex(regs->err_code);
        vga_writeln("\n系统已停止。");

        for (;;) {
            asm volatile("hlt");
        }
    }
}

void irq_handler(regs_t *regs)
{
    pic_send_eoi(regs->int_no - 32);

    if (isr_handlers[regs->int_no] != 0) {
        isr_handlers[regs->int_no](regs);
    }
}

void isr_register_handler(uint8_t vector, isr_handler_t handler)
{
    isr_handlers[vector] = handler;
}

void idt_init(void)
{
    for (int i = 0; i < 256; i++) {
        isr_handlers[i] = 0;
    }

    idt_set_entry(0,  (uint64_t)isr0,  0x08, 0, IDT_GATE_INTERRUPT | IDT_RING0);
    idt_set_entry(1,  (uint64_t)isr1,  0x08, 0, IDT_GATE_INTERRUPT | IDT_RING0);
    idt_set_entry(2,  (uint64_t)isr2,  0x08, 0, IDT_GATE_INTERRUPT | IDT_RING0);
    idt_set_entry(3,  (uint64_t)isr3,  0x08, 0, IDT_GATE_INTERRUPT | IDT_RING0);
    idt_set_entry(4,  (uint64_t)isr4,  0x08, 0, IDT_GATE_INTERRUPT | IDT_RING0);
    idt_set_entry(5,  (uint64_t)isr5,  0x08, 0, IDT_GATE_INTERRUPT | IDT_RING0);
    idt_set_entry(6,  (uint64_t)isr6,  0x08, 0, IDT_GATE_INTERRUPT | IDT_RING0);
    idt_set_entry(7,  (uint64_t)isr7,  0x08, 0, IDT_GATE_INTERRUPT | IDT_RING0);
    idt_set_entry(8,  (uint64_t)isr8,  0x08, 0, IDT_GATE_INTERRUPT | IDT_RING0);
    idt_set_entry(9,  (uint64_t)isr9,  0x08, 0, IDT_GATE_INTERRUPT | IDT_RING0);
    idt_set_entry(10, (uint64_t)isr10, 0x08, 0, IDT_GATE_INTERRUPT | IDT_RING0);
    idt_set_entry(11, (uint64_t)isr11, 0x08, 0, IDT_GATE_INTERRUPT | IDT_RING0);
    idt_set_entry(12, (uint64_t)isr12, 0x08, 0, IDT_GATE_INTERRUPT | IDT_RING0);
    idt_set_entry(13, (uint64_t)isr13, 0x08, 0, IDT_GATE_INTERRUPT | IDT_RING0);
    idt_set_entry(14, (uint64_t)isr14, 0x08, 0, IDT_GATE_INTERRUPT | IDT_RING0);
    idt_set_entry(15, (uint64_t)isr15, 0x08, 0, IDT_GATE_INTERRUPT | IDT_RING0);
    idt_set_entry(16, (uint64_t)isr16, 0x08, 0, IDT_GATE_INTERRUPT | IDT_RING0);
    idt_set_entry(17, (uint64_t)isr17, 0x08, 0, IDT_GATE_INTERRUPT | IDT_RING0);
    idt_set_entry(18, (uint64_t)isr18, 0x08, 0, IDT_GATE_INTERRUPT | IDT_RING0);
    idt_set_entry(19, (uint64_t)isr19, 0x08, 0, IDT_GATE_INTERRUPT | IDT_RING0);
    idt_set_entry(20, (uint64_t)isr20, 0x08, 0, IDT_GATE_INTERRUPT | IDT_RING0);
    idt_set_entry(21, (uint64_t)isr21, 0x08, 0, IDT_GATE_INTERRUPT | IDT_RING0);
    idt_set_entry(22, (uint64_t)isr22, 0x08, 0, IDT_GATE_INTERRUPT | IDT_RING0);
    idt_set_entry(23, (uint64_t)isr23, 0x08, 0, IDT_GATE_INTERRUPT | IDT_RING0);
    idt_set_entry(24, (uint64_t)isr24, 0x08, 0, IDT_GATE_INTERRUPT | IDT_RING0);
    idt_set_entry(25, (uint64_t)isr25, 0x08, 0, IDT_GATE_INTERRUPT | IDT_RING0);
    idt_set_entry(26, (uint64_t)isr26, 0x08, 0, IDT_GATE_INTERRUPT | IDT_RING0);
    idt_set_entry(27, (uint64_t)isr27, 0x08, 0, IDT_GATE_INTERRUPT | IDT_RING0);
    idt_set_entry(28, (uint64_t)isr28, 0x08, 0, IDT_GATE_INTERRUPT | IDT_RING0);
    idt_set_entry(29, (uint64_t)isr29, 0x08, 0, IDT_GATE_INTERRUPT | IDT_RING0);
    idt_set_entry(30, (uint64_t)isr30, 0x08, 0, IDT_GATE_INTERRUPT | IDT_RING0);
    idt_set_entry(31, (uint64_t)isr31, 0x08, 0, IDT_GATE_INTERRUPT | IDT_RING0);

    idt_set_entry(32, (uint64_t)irq0,  0x08, 0, IDT_GATE_INTERRUPT | IDT_RING0);
    idt_set_entry(33, (uint64_t)irq1,  0x08, 0, IDT_GATE_INTERRUPT | IDT_RING0);
    idt_set_entry(34, (uint64_t)irq2,  0x08, 0, IDT_GATE_INTERRUPT | IDT_RING0);
    idt_set_entry(35, (uint64_t)irq3,  0x08, 0, IDT_GATE_INTERRUPT | IDT_RING0);
    idt_set_entry(36, (uint64_t)irq4,  0x08, 0, IDT_GATE_INTERRUPT | IDT_RING0);
    idt_set_entry(37, (uint64_t)irq5,  0x08, 0, IDT_GATE_INTERRUPT | IDT_RING0);
    idt_set_entry(38, (uint64_t)irq6,  0x08, 0, IDT_GATE_INTERRUPT | IDT_RING0);
    idt_set_entry(39, (uint64_t)irq7,  0x08, 0, IDT_GATE_INTERRUPT | IDT_RING0);
    idt_set_entry(40, (uint64_t)irq8,  0x08, 0, IDT_GATE_INTERRUPT | IDT_RING0);
    idt_set_entry(41, (uint64_t)irq9,  0x08, 0, IDT_GATE_INTERRUPT | IDT_RING0);
    idt_set_entry(42, (uint64_t)irq10, 0x08, 0, IDT_GATE_INTERRUPT | IDT_RING0);
    idt_set_entry(43, (uint64_t)irq11, 0x08, 0, IDT_GATE_INTERRUPT | IDT_RING0);
    idt_set_entry(44, (uint64_t)irq12, 0x08, 0, IDT_GATE_INTERRUPT | IDT_RING0);
    idt_set_entry(45, (uint64_t)irq13, 0x08, 0, IDT_GATE_INTERRUPT | IDT_RING0);
    idt_set_entry(46, (uint64_t)irq14, 0x08, 0, IDT_GATE_INTERRUPT | IDT_RING0);
    idt_set_entry(47, (uint64_t)irq15, 0x08, 0, IDT_GATE_INTERRUPT | IDT_RING0);

    idt_set_entry(128, (uint64_t)isr128, 0x08, 0, IDT_GATE_INTERRUPT | IDT_RING3);
    idt_set_entry(129, (uint64_t)isr129, 0x08, 0, IDT_GATE_INTERRUPT | IDT_RING0);

    idt_ptr.limit = sizeof(idt) - 1;
    idt_ptr.base  = (uint64_t)&idt;

    load_idt((uint64_t)&idt_ptr);

    serial_write("[IDT] IDT 初始化完成 (256 条目)\n");
}