#include "mouse.h"
#include "hal.h"
#include "irq.h"
#include "idt.h"
#include "pic.h"
#include "fb.h"
#include "serial.h"

#define MOUSE_PORT_DATA   0x60
#define MOUSE_PORT_STATUS 0x64
#define MOUSE_PORT_CMD    0x64

#define MOUSE_CMD_ENABLE_AUX  0xA8
#define MOUSE_CMD_GET_COMPAQ  0x20
#define MOUSE_CMD_SET_COMPAQ  0x60
#define MOUSE_CMD_WRITE_AUX   0xD4
#define MOUSE_CMD_RESET       0xFF
#define MOUSE_CMD_DEFAULTS    0xF6
#define MOUSE_CMD_ENABLE_RPT  0xF4

mouse_state_t g_mouse;
static int     mouse_cycle = 0;
static uint8_t mouse_bytes[3];
static int     last_x, last_y;
static bool    cursor_visible = false;

static void mouse_wait_write(void)
{
    uint32_t timeout = 100000;
    while ((inb(MOUSE_PORT_STATUS) & 2) && --timeout) {
        asm volatile("pause");
    }
}

static void mouse_wait_read(void)
{
    uint32_t timeout = 100000;
    while (!(inb(MOUSE_PORT_STATUS) & 1) && --timeout) {
        asm volatile("pause");
    }
}

static void mouse_write(uint8_t data)
{
    mouse_wait_write();
    outb(MOUSE_PORT_CMD, MOUSE_CMD_WRITE_AUX);
    mouse_wait_write();
    outb(MOUSE_PORT_DATA, data);
}

static uint8_t mouse_read(void)
{
    mouse_wait_read();
    return inb(MOUSE_PORT_DATA);
}

static void mouse_handler(regs_t *regs)
{
    (void)regs;

    uint8_t status = inb(MOUSE_PORT_STATUS);
    if (!(status & 0x20)) return;

    uint8_t data = inb(MOUSE_PORT_DATA);

    switch (mouse_cycle) {
    case 0:
        if (!(data & 0x08)) return;
        mouse_bytes[0] = data;
        mouse_cycle = 1;
        break;
    case 1:
        mouse_bytes[1] = data;
        mouse_cycle = 2;
        break;
    case 2:
        mouse_bytes[2] = data;
        mouse_cycle = 0;

        g_mouse.left   = (mouse_bytes[0] & 1) != 0;
        g_mouse.right  = (mouse_bytes[0] & 2) != 0;
        g_mouse.middle = (mouse_bytes[0] & 4) != 0;

        int dx = (int)mouse_bytes[1];
        int dy = (int)mouse_bytes[2];

        if (mouse_bytes[0] & 0x10) dx |= ~0xFF;
        if (mouse_bytes[0] & 0x20) dy |= ~0xFF;

        g_mouse.x += dx;
        g_mouse.y -= dy;

        int max_x = (int)fb_get_width() - 1;
        int max_y = (int)fb_get_height() - 1;
        if (g_mouse.x < 0) g_mouse.x = 0;
        if (g_mouse.y < 0) g_mouse.y = 0;
        if (g_mouse.x > max_x) g_mouse.x = max_x;
        if (g_mouse.y > max_y) g_mouse.y = max_y;

        g_mouse.valid = true;
        break;
    }
}

void mouse_init(void)
{
    serial_write("[MOUSE] 初始化 PS/2 鼠标...\n");

    g_mouse.x     = 400;
    g_mouse.y     = 300;
    g_mouse.left  = false;
    g_mouse.right = false;
    g_mouse.middle = false;
    g_mouse.valid = false;
    mouse_cycle   = 0;
    last_x = g_mouse.x;
    last_y = g_mouse.y;
    cursor_visible = false;

    mouse_wait_write();
    outb(MOUSE_PORT_CMD, MOUSE_CMD_ENABLE_AUX);

    mouse_wait_write();
    outb(MOUSE_PORT_CMD, MOUSE_CMD_GET_COMPAQ);
    mouse_wait_read();
    uint8_t config = inb(MOUSE_PORT_DATA);

    config |= 2;
    config &= ~0x20;

    mouse_wait_write();
    outb(MOUSE_PORT_CMD, MOUSE_CMD_SET_COMPAQ);
    mouse_wait_write();
    outb(MOUSE_PORT_DATA, config);

    irq_register_handler(IRQ_MOUSE, mouse_handler);
    irq_enable(IRQ_MOUSE);

    mouse_write(MOUSE_CMD_DEFAULTS);
    mouse_read();

    mouse_write(MOUSE_CMD_ENABLE_RPT);
    mouse_read();

    serial_write("[MOUSE] PS/2 鼠标初始化完成\n");
}

void mouse_poll(void)
{
    (void)0;
}

void mouse_get_state(mouse_state_t *state)
{
    if (state) *state = g_mouse;
}

static const uint8_t cursor_bmp[] = {
    1,0,0,0,0,0,0,0,0,0,0,0,
    1,1,0,0,0,0,0,0,0,0,0,0,
    1,2,1,0,0,0,0,0,0,0,0,0,
    1,2,2,1,0,0,0,0,0,0,0,0,
    1,2,2,2,1,0,0,0,0,0,0,0,
    1,2,2,2,2,1,0,0,0,0,0,0,
    1,2,2,2,2,2,1,0,0,0,0,0,
    1,2,2,2,2,2,2,1,0,0,0,0,
    1,2,2,2,2,2,2,2,1,0,0,0,
    1,2,2,2,2,2,2,2,2,1,0,0,
    1,2,2,2,2,2,2,2,2,2,1,0,
    1,2,2,2,2,2,1,0,0,0,0,0,
    1,1,2,2,1,2,2,1,0,0,0,0,
    1,0,1,1,0,1,2,2,1,0,0,0,
    0,0,0,0,0,1,2,2,1,0,0,0,
    0,0,0,0,0,1,2,2,2,1,0,0,
    0,0,0,0,0,0,1,2,2,1,0,0,
    0,0,0,0,0,0,1,2,2,2,1,0,
    0,0,0,0,0,0,0,1,2,2,1,0,
};
#define CURSOR_W 12
#define CURSOR_H 19

static void cursor_restore(void)
{
    for (int yy = 0; yy < CURSOR_H; yy++) {
        for (int xx = 0; xx < CURSOR_W; xx++) {
            uint8_t px = cursor_bmp[yy * CURSOR_W + xx];
            if (px == 1 || px == 2) {
                fb_putpixel((uint32_t)(last_x + xx), (uint32_t)(last_y + yy),
                            fb_getpixel((uint32_t)(last_x + xx), (uint32_t)(last_y + yy)) ^ 0x00FFFFFF);
            }
        }
    }
}

void mouse_draw_cursor(void)
{
    if (!fb_available()) return;

    for (int yy = 0; yy < CURSOR_H; yy++) {
        for (int xx = 0; xx < CURSOR_W; xx++) {
            uint8_t px = cursor_bmp[yy * CURSOR_W + xx];
            if (px == 0) continue;

            uint32_t color;
            if (px == 1) {
                color = 0x00000000;
            } else {
                color = 0x00FFFFFF;
            }

            uint32_t sx = (uint32_t)(g_mouse.x + xx);
            uint32_t sy = (uint32_t)(g_mouse.y + yy);
            uint32_t orig = fb_getpixel(sx, sy);
            fb_putpixel(sx, sy, (px == 1) ? (orig ^ 0x00FFFFFF) : color);
            if (px == 1) {
                fb_putpixel(sx, sy, orig ^ 0x00FFFFFF);
            } else {
                fb_putpixel(sx, sy, color);
            }
        }
    }
}

void mouse_clear_cursor(void)
{
    if (!cursor_visible || !fb_available()) return;
    cursor_restore();
    cursor_visible = false;
}

void mouse_update_and_draw(void)
{
    if (!g_mouse.valid) return;

    if (cursor_visible) {
        cursor_restore();
    }

    mouse_draw_cursor();
    last_x = g_mouse.x;
    last_y = g_mouse.y;
    cursor_visible = true;
    g_mouse.valid = false;
}

void mouse_draw_at(void)
{
    if (!g_mouse.valid || !fb_available()) return;

    cursor_visible = false;
    mouse_draw_cursor();
    last_x = g_mouse.x;
    last_y = g_mouse.y;
    cursor_visible = true;
    g_mouse.valid = false;
}