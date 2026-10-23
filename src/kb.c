#include "kb.h"
#include "hal.h"
#include "idt.h"
#include "pic.h"
#include "irq.h"
#include "serial.h"

#define KB_BUFFER_SIZE 256

static uint8_t  kb_buffer[KB_BUFFER_SIZE];
static uint32_t kb_head = 0;
static uint32_t kb_tail = 0;

static const uint8_t scancode_lower[] = {
    0,    0x1B, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0,
    'a',  's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\',
    'z',  'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' ', 0,
};

static const uint8_t scancode_upper[] = {
    0,    0x1B, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', 0,
    'A',  'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0, '|',
    'Z',  'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*', 0, ' ', 0,
};

static bool lshift = false;
static bool rshift = false;

static void kb_push(uint8_t c)
{
    uint32_t next = (kb_head + 1) % KB_BUFFER_SIZE;
    if (next != kb_tail) {
        kb_buffer[kb_head] = c;
        kb_head = next;
    }
}

static void kb_handler(regs_t *regs)
{
    (void)regs;
    uint8_t scancode = inb(0x60);

    if (scancode == 0x2A) {
        lshift = true;
    } else if (scancode == 0xAA) {
        lshift = false;
    } else if (scancode == 0x36) {
        rshift = true;
    } else if (scancode == 0xB6) {
        rshift = false;
    } else if (scancode < 0x80) {
        bool shift = lshift || rshift;
        if (scancode < sizeof(scancode_lower)) {
            uint8_t ch = shift ? scancode_upper[scancode] : scancode_lower[scancode];
            if (ch != 0) {
                kb_push(ch);
            }
        }
    }
}

void kb_init(void)
{
    kb_head = 0;
    kb_tail = 0;
    lshift  = false;
    rshift  = false;
    irq_register_handler(IRQ_KEYBOARD, kb_handler);
    irq_enable(IRQ_KEYBOARD);
    serial_write("[KB] 键盘驱动初始化完成 (IRQ1)\n");
}

char kb_getchar(void)
{
    while (kb_head == kb_tail) {
        asm volatile("hlt");
    }
    uint8_t c = kb_buffer[kb_tail];
    kb_tail = (kb_tail + 1) % KB_BUFFER_SIZE;
    return (char)c;
}

bool kb_haschar(void)
{
    return kb_head != kb_tail;
}