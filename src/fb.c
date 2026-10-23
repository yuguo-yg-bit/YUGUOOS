#include "fb.h"
#include "font.h"
#include "serial.h"

struct multiboot2_tag {
    uint32_t type;
    uint32_t size;
} __attribute__((packed));

#define MULTIBOOT2_TAG_FB 8

struct multiboot2_fb_tag {
    uint32_t type;
    uint32_t size;
    uint64_t addr;
    uint32_t pitch;
    uint32_t width;
    uint32_t height;
    uint8_t  bpp;
    uint8_t  fb_type;
    uint16_t reserved;
    uint8_t  red_pos;
    uint8_t  red_mask;
    uint8_t  green_pos;
    uint8_t  green_mask;
    uint8_t  blue_pos;
    uint8_t  blue_mask;
} __attribute__((packed));

static struct fb_info fb;
static bool fb_ok = false;

static inline void fb_write_pixel(uint32_t x, uint32_t y, uint32_t color)
{
    uint32_t off = y * fb.pitch + x * fb.bpp_bytes;
    if (fb.bpp_bytes == 4) {
        fb.addr[off]     = (uint8_t)(color & 0xFF);
        fb.addr[off + 1] = (uint8_t)((color >> 8) & 0xFF);
        fb.addr[off + 2] = (uint8_t)((color >> 16) & 0xFF);
        fb.addr[off + 3] = (uint8_t)((color >> 24) & 0xFF);
    } else if (fb.bpp_bytes == 3) {
        fb.addr[off]     = (uint8_t)(color & 0xFF);
        fb.addr[off + 1] = (uint8_t)((color >> 8) & 0xFF);
        fb.addr[off + 2] = (uint8_t)((color >> 16) & 0xFF);
    }
}

void fb_init(uintptr_t multiboot_addr)
{
    struct multiboot2_tag *tag = (struct multiboot2_tag *)(uintptr_t)(multiboot_addr + 8);

    while (tag->type != 0) {
        if (tag->type == MULTIBOOT2_TAG_FB) {
            struct multiboot2_fb_tag *fbt = (struct multiboot2_fb_tag *)tag;
            fb.addr      = (uint8_t *)(uintptr_t)fbt->addr;
            fb.pitch     = fbt->pitch;
            fb.width     = fbt->width;
            fb.height    = fbt->height;
            fb.bpp       = fbt->bpp;
            fb.type      = fbt->fb_type;
            fb.bpp_bytes = fbt->bpp / 8;
            fb.red_pos   = fbt->red_pos;
            fb.red_mask  = fbt->red_mask;
            fb.green_pos = fbt->green_pos;
            fb.green_mask = fbt->green_mask;
            fb.blue_pos  = fbt->blue_pos;
            fb.blue_mask = fbt->blue_mask;
            fb_ok        = true;

            serial_write("[FB] 帧缓冲: ");
            serial_write_hex(fb.width);
            serial_write("x");
            serial_write_hex(fb.height);
            serial_write("x");
            serial_write_hex(fb.bpp);
            serial_write(" pitch=");
            serial_write_hex(fb.pitch);
            serial_write(" type=");
            serial_write_hex(fb.type);
            serial_write(" addr=0x");
            serial_write_hex((uint32_t)(fbt->addr >> 32));
            serial_write_hex((uint32_t)(fbt->addr & 0xFFFFFFFF));
            serial_write("\n");
            return;
        }
        tag = (struct multiboot2_tag *)((uint8_t *)tag + ((tag->size + 7) & ~7));
    }

    serial_write("[FB] 未找到帧缓冲信息，使用 VGA 文本模式\n");
}

bool fb_available(void)
{
    return fb_ok;
}

uint32_t fb_get_width(void)  { return fb.width; }
uint32_t fb_get_height(void) { return fb.height; }

void fb_clear(uint32_t color)
{
    if (!fb_ok) return;
    uint8_t b0 = (uint8_t)(color & 0xFF);
    uint8_t b1 = (uint8_t)((color >> 8) & 0xFF);
    uint8_t b2 = (uint8_t)((color >> 16) & 0xFF);
    uint8_t b3 = (uint8_t)((color >> 24) & 0xFF);

    if (fb.bpp_bytes == 4) {
        uint32_t *p = (uint32_t *)fb.addr;
        uint32_t w32 = (uint32_t)b0 | ((uint32_t)b1 << 8) | ((uint32_t)b2 << 16) | ((uint32_t)b3 << 24);
        uint32_t count = (fb.pitch / 4) * fb.height;
        for (uint32_t i = 0; i < count; i++) {
            p[i] = w32;
        }
    } else if (fb.bpp_bytes == 3) {
        for (uint32_t y = 0; y < fb.height; y++) {
            uint8_t *row = fb.addr + y * fb.pitch;
            for (uint32_t x = 0; x < fb.width; x++) {
                row[x * 3]     = b0;
                row[x * 3 + 1] = b1;
                row[x * 3 + 2] = b2;
            }
        }
    }
}

void fb_putpixel(uint32_t x, uint32_t y, uint32_t color)
{
    if (!fb_ok || x >= fb.width || y >= fb.height) return;
    fb_write_pixel(x, y, color);
}

uint32_t fb_getpixel(uint32_t x, uint32_t y)
{
    if (!fb_ok || x >= fb.width || y >= fb.height) return 0;
    uint32_t off = y * fb.pitch + x * fb.bpp_bytes;
    if (fb.bpp_bytes == 4) {
        return (uint32_t)fb.addr[off] | ((uint32_t)fb.addr[off + 1] << 8) |
               ((uint32_t)fb.addr[off + 2] << 16) | ((uint32_t)fb.addr[off + 3] << 24);
    } else if (fb.bpp_bytes == 3) {
        return (uint32_t)fb.addr[off] | ((uint32_t)fb.addr[off + 1] << 8) |
               ((uint32_t)fb.addr[off + 2] << 16);
    }
    return 0;
}

void fb_draw_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color)
{
    if (!fb_ok) return;
    for (uint32_t i = 0; i < w; i++) {
        fb_putpixel(x + i, y, color);
        fb_putpixel(x + i, y + h - 1, color);
    }
    for (uint32_t i = 0; i < h; i++) {
        fb_putpixel(x, y + i, color);
        fb_putpixel(x + w - 1, y + i, color);
    }
}

void fb_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color)
{
    if (!fb_ok || x + w > fb.width || y + h > fb.height) return;

    if (fb.bpp_bytes == 4) {
        uint32_t color32 = (color & 0xFF) | (((color >> 8) & 0xFF) << 8) |
                           (((color >> 16) & 0xFF) << 16) | (((color >> 24) & 0xFF) << 24);
        for (uint32_t yy = 0; yy < h; yy++) {
            uint32_t *row = (uint32_t *)(fb.addr + (y + yy) * fb.pitch);
            for (uint32_t xx = 0; xx < w; xx++) {
                row[x + xx] = color32;
            }
        }
    } else {
        for (uint32_t yy = 0; yy < h; yy++) {
            for (uint32_t xx = 0; xx < w; xx++) {
                fb_write_pixel(x + xx, y + yy, color);
            }
        }
    }
}

void fb_draw_char(uint32_t x, uint32_t y, char c, uint32_t fg, uint32_t bg)
{
    if (!fb_ok) return;
    uint8_t idx = (uint8_t)c - 32;
    if (idx >= FONT_CHARS) idx = 0;

    for (int row = 0; row < FONT_H; row++) {
        uint8_t bits = font8x16[idx][row];
        for (int col = 0; col < 8; col++) {
            uint32_t color = (bits & (0x80 >> col)) ? fg : bg;
            fb_putpixel(x + col, y + row, color);
        }
    }
}

void fb_draw_string(uint32_t x, uint32_t y, const char *str, uint32_t fg, uint32_t bg)
{
    if (!fb_ok) return;
    uint32_t cx = x;
    for (; *str; str++) {
        if (*str == '\n') {
            cx = x;
            y += FONT_H;
            continue;
        }
        fb_draw_char(cx, y, *str, fg, bg);
        cx += FB_CHAR_W;
    }
}

void fb_draw_string_centered(uint32_t y, const char *str, uint32_t fg, uint32_t bg)
{
    if (!fb_ok) return;
    size_t len = 0;
    for (const char *p = str; *p; p++) len++;
    uint32_t x = (fb.width - (uint32_t)len * FB_CHAR_W) / 2;
    fb_draw_string(x, y, str, fg, bg);
}

void fb_draw_big_char(uint32_t x, uint32_t y, char c, uint32_t fg)
{
    if (!fb_ok) return;
    uint8_t idx = (uint8_t)c - 32;
    if (idx >= FONT_CHARS) idx = 0;

    int scale = 2;
    for (int row = 0; row < FONT_H; row++) {
        uint8_t bits = font8x16[idx][row];
        for (int col = 0; col < 8; col++) {
            if (bits & (0x80 >> col)) {
                for (int sy = 0; sy < scale; sy++) {
                    for (int sx = 0; sx < scale; sx++) {
                        fb_putpixel(x + col * scale + sx, y + row * scale + sy, fg);
                    }
                }
            }
        }
    }
}

void fb_draw_big_string(uint32_t x, uint32_t y, const char *str, uint32_t fg)
{
    if (!fb_ok) return;
    for (; *str; str++) {
        fb_draw_big_char(x, y, *str, fg);
        x += FB_BIG_CHAR_W;
    }
}

void fb_draw_big_string_centered(uint32_t y, const char *str, uint32_t fg)
{
    if (!fb_ok) return;
    size_t len = 0;
    for (const char *p = str; *p; p++) len++;
    uint32_t x = (fb.width - (uint32_t)len * FB_BIG_CHAR_W) / 2;
    fb_draw_big_string(x, y, str, fg);
}

void fb_draw_huge_char(uint32_t x, uint32_t y, char c, uint32_t fg)
{
    if (!fb_ok) return;
    uint8_t idx = (uint8_t)c - 32;
    if (idx >= FONT_CHARS) idx = 0;

    int scale = 4;
    for (int row = 0; row < FONT_H; row++) {
        uint8_t bits = font8x16[idx][row];
        for (int col = 0; col < 8; col++) {
            if (bits & (0x80 >> col)) {
                for (int sy = 0; sy < scale; sy++) {
                    for (int sx = 0; sx < scale; sx++) {
                        fb_putpixel(x + col * scale + sx, y + row * scale + sy, fg);
                    }
                }
            }
        }
    }
}

void fb_draw_huge_string(uint32_t x, uint32_t y, const char *str, uint32_t fg)
{
    if (!fb_ok) return;
    for (; *str; str++) {
        fb_draw_huge_char(x, y, *str, fg);
        x += FB_HUGE_CHAR_W;
    }
}

void fb_draw_huge_string_centered(uint32_t y, const char *str, uint32_t fg)
{
    if (!fb_ok) return;
    size_t len = 0;
    for (const char *p = str; *p; p++) len++;
    uint32_t x = (fb.width - (uint32_t)len * FB_HUGE_CHAR_W) / 2;
    fb_draw_huge_string(x, y, str, fg);
}