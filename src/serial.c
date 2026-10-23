#include "serial.h"
#include "hal.h"

#define COM1_PORT         0x3F8
#define COM1_DATA         (COM1_PORT + 0)
#define COM1_INT_ENABLE   (COM1_PORT + 1)
#define COM1_FIFO_CTRL    (COM1_PORT + 2)
#define COM1_LINE_CTRL    (COM1_PORT + 3)
#define COM1_MODEM_CTRL   (COM1_PORT + 4)
#define COM1_LINE_STATUS  (COM1_PORT + 5)

static int serial_ready;

void serial_init(void)
{
    outb(COM1_INT_ENABLE, 0x00);
    outb(COM1_LINE_CTRL, 0x80);
    outb(COM1_DATA, 0x03);
    outb(COM1_INT_ENABLE, 0x00);
    outb(COM1_LINE_CTRL, 0x03);
    outb(COM1_FIFO_CTRL, 0xC7);
    outb(COM1_MODEM_CTRL, 0x0B);
    outb(COM1_MODEM_CTRL, 0x0F);
    serial_ready = 1;
}

static int serial_is_transmit_empty(void)
{
    return inb(COM1_LINE_STATUS) & 0x20;
}

void serial_putchar(char c)
{
    if (!serial_ready) return;
    while (!serial_is_transmit_empty()) {}
    outb(COM1_DATA, (uint8_t)c);
}

void serial_write(const char *str)
{
    if (!serial_ready) return;
    for (; *str != '\0'; str++) {
        if (*str == '\n') {
            serial_putchar('\r');
        }
        serial_putchar(*str);
    }
}

void serial_write_hex(uint64_t value)
{
    if (!serial_ready) return;
    char buf[19];
    buf[18] = '\0';
    int pos = 17;
    if (value == 0) {
        buf[pos--] = '0';
    } else {
        while (value > 0 && pos >= 0) {
            uint8_t digit = value & 0xF;
            buf[pos--] = (digit < 10) ? ('0' + digit) : ('A' + digit - 10);
            value >>= 4;
        }
    }
    serial_write(&buf[pos + 1]);
}

void serial_write_dec(uint64_t value)
{
    if (!serial_ready) return;
    char buf[21];
    buf[20] = '\0';
    int pos = 19;
    if (value == 0) {
        buf[pos--] = '0';
    } else {
        while (value > 0 && pos >= 0) {
            buf[pos--] = '0' + (value % 10);
            value /= 10;
        }
    }
    serial_write(&buf[pos + 1]);
}