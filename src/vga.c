#include "vga.h"

static volatile uint16_t *const vga_buffer = (uint16_t *)VGA_BUFFER;
static uint8_t vga_row = 0;
static uint8_t vga_col = 0;
static uint8_t vga_color = 0;
static uint8_t vga_fg = VGA_LIGHT_GREY;
static uint8_t vga_bg = VGA_BLACK;

void vga_init(void)
{
    vga_fg = VGA_LIGHT_GREY;
    vga_bg = VGA_BLACK;
    vga_color = (vga_bg << 4) | vga_fg;
    vga_row = 0;
    vga_col = 0;
    vga_clear();
}

void vga_clear(void)
{
    uint16_t blank = (uint16_t)' ' | ((uint16_t)vga_color << 8);
    for (size_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_buffer[i] = blank;
    }
    vga_row = 0;
    vga_col = 0;
}

void vga_set_color(uint8_t fg, uint8_t bg)
{
    vga_fg = fg;
    vga_bg = bg;
    vga_color = (bg << 4) | fg;
}

void vga_putchar(char c)
{
    if (c == '\n') {
        vga_col = 0;
        vga_row++;
    } else if (c == '\r') {
        vga_col = 0;
    } else if (c == '\t') {
        vga_col = (vga_col + 4) & ~3;
        if (vga_col >= VGA_WIDTH) {
            vga_col = 0;
            vga_row++;
        }
    } else if (c == '\b') {
        if (vga_col > 0) {
            vga_col--;
        }
    } else {
        size_t idx = (size_t)vga_row * VGA_WIDTH + vga_col;
        vga_buffer[idx] = (uint16_t)c | ((uint16_t)vga_color << 8);
        vga_col++;
    }

    if (vga_col >= VGA_WIDTH) {
        vga_col = 0;
        vga_row++;
    }
    if (vga_row >= VGA_HEIGHT) {
        vga_scroll();
    }
}

void vga_write(const char *str)
{
    while (*str) {
        vga_putchar(*str++);
    }
}

void vga_writeln(const char *str)
{
    vga_write(str);
    vga_putchar('\n');
}

static const char hex_chars[] = "0123456789ABCDEF";

void vga_write_hex(uint64_t value)
{
    vga_write("0x");
    bool started = false;
    for (int i = 60; i >= 0; i -= 4) {
        uint8_t nibble = (value >> i) & 0xF;
        if (nibble != 0 || started || i == 0) {
            vga_putchar(hex_chars[nibble]);
            started = true;
        }
    }
}

void vga_scroll(void)
{
    for (size_t y = 0; y < VGA_HEIGHT - 1; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            vga_buffer[y * VGA_WIDTH + x] = vga_buffer[(y + 1) * VGA_WIDTH + x];
        }
    }
    uint16_t blank = (uint16_t)' ' | ((uint16_t)vga_color << 8);
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        vga_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = blank;
    }
    vga_row = VGA_HEIGHT - 1;
    vga_col = 0;
}

void vga_set_cursor(uint8_t x, uint8_t y)
{
    vga_col = x;
    vga_row = y;
}