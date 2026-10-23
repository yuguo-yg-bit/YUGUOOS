#include "canvas.h"
#include "fb.h"
#include "font.h"
#include "vmm.h"

canvas_t *canvas_create(uint32_t w, uint32_t h)
{
    canvas_t *c = (canvas_t *)kmalloc(sizeof(canvas_t));
    if (!c) return NULL;

    c->width  = w;
    c->height = h;
    c->pitch  = w;
    c->is_external = false;
    c->pixels = (color_t *)kmalloc(w * h * sizeof(color_t));
    if (!c->pixels) {
        kfree(c);
        return NULL;
    }
    canvas_clear(c, COLOR_BLACK);
    return c;
}

void canvas_destroy(canvas_t *c)
{
    if (!c) return;
    if (!c->is_external && c->pixels) {
        kfree(c->pixels);
    }
    kfree(c);
}

void canvas_clear(canvas_t *c, color_t color)
{
    if (!c || !c->pixels) return;
    uint32_t count = c->pitch * c->height;
    for (uint32_t i = 0; i < count; i++) {
        c->pixels[i] = color;
    }
}

static inline bool clip_pixel(canvas_t *c, int x, int y)
{
    return (x >= 0 && y >= 0 && (uint32_t)x < c->width && (uint32_t)y < c->height);
}

void canvas_putpixel(canvas_t *c, int x, int y, color_t color)
{
    if (!c || !c->pixels) return;
    if (!clip_pixel(c, x, y)) return;
    c->pixels[y * c->pitch + x] = color;
}

color_t canvas_getpixel(canvas_t *c, int x, int y)
{
    if (!c || !c->pixels) return 0;
    if (!clip_pixel(c, x, y)) return 0;
    return c->pixels[y * c->pitch + x];
}

void canvas_fill_rect(canvas_t *c, int x, int y, int w, int h, color_t color)
{
    if (!c || !c->pixels || w <= 0 || h <= 0) return;

    int x0 = x, y0 = y;
    int x1 = x + w, y1 = y + h;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > (int)c->width)  x1 = (int)c->width;
    if (y1 > (int)c->height) y1 = (int)c->height;
    if (x0 >= x1 || y0 >= y1) return;

    for (int yy = y0; yy < y1; yy++) {
        color_t *row = c->pixels + yy * c->pitch;
        for (int xx = x0; xx < x1; xx++) {
            row[xx] = color;
        }
    }
}

void canvas_draw_rect(canvas_t *c, int x, int y, int w, int h, color_t color)
{
    if (!c || !c->pixels || w <= 0 || h <= 0) return;
    canvas_draw_hline(c, x, y, w, color);
    canvas_draw_hline(c, x, y + h - 1, w, color);
    canvas_draw_vline(c, x, y, h, color);
    canvas_draw_vline(c, x + w - 1, y, h, color);
}

void canvas_draw_hline(canvas_t *c, int x, int y, int len, color_t color)
{
    if (!c || !c->pixels) return;
    if (!clip_pixel(c, x, y)) return;

    int x0 = x, x1 = x + len;
    if (x0 < 0) x0 = 0;
    if (x1 > (int)c->width) x1 = (int)c->width;
    if (x0 >= x1) return;

    color_t *row = c->pixels + y * c->pitch;
    for (int xx = x0; xx < x1; xx++) {
        row[xx] = color;
    }
}

void canvas_draw_vline(canvas_t *c, int x, int y, int len, color_t color)
{
    if (!c || !c->pixels) return;
    if (!clip_pixel(c, x, y)) return;

    int y0 = y, y1 = y + len;
    if (y0 < 0) y0 = 0;
    if (y1 > (int)c->height) y1 = (int)c->height;
    if (y0 >= y1) return;

    for (int yy = y0; yy < y1; yy++) {
        c->pixels[yy * c->pitch + x] = color;
    }
}

void canvas_blit(canvas_t *dst, int dx, int dy, canvas_t *src, int sx, int sy, int w, int h)
{
    if (!dst || !src || !dst->pixels || !src->pixels) return;
    if (w <= 0 || h <= 0) return;

    for (int yy = 0; yy < h; yy++) {
        int src_y = sy + yy;
        int dst_y = dy + yy;
        if (src_y < 0 || (uint32_t)src_y >= src->height) continue;
        if (dst_y < 0 || (uint32_t)dst_y >= dst->height) continue;

        color_t *src_row = src->pixels + src_y * src->pitch;
        color_t *dst_row = dst->pixels + dst_y * dst->pitch;

        for (int xx = 0; xx < w; xx++) {
            int src_x = sx + xx;
            int dst_x = dx + xx;
            if (src_x < 0 || (uint32_t)src_x >= src->width) continue;
            if (dst_x < 0 || (uint32_t)dst_x >= dst->width) continue;
            dst_row[dst_x] = src_row[src_x];
        }
    }
}

void canvas_draw_char(canvas_t *c, int x, int y, char ch, color_t fg, color_t bg)
{
    if (!c || !c->pixels) return;

    uint8_t idx = (uint8_t)ch;
    if (idx < 32) idx = 0;
    else idx -= 32;
    if (idx >= FONT_CHARS) idx = 0;

    for (int row = 0; row < FONT_H; row++) {
        uint8_t bits = font8x16[idx][row];
        for (int col = 0; col < 8; col++) {
            color_t clr = (bits & (0x80 >> col)) ? fg : bg;
            canvas_putpixel(c, x + col, y + row, clr);
        }
    }
}

void canvas_draw_string(canvas_t *c, int x, int y, const char *str, color_t fg, color_t bg)
{
    if (!c || !str) return;
    int cx = x;
    while (*str) {
        canvas_draw_char(c, cx, y, *str, fg, bg);
        cx += CANVAS_CHAR_W;
        str++;
    }
}

canvas_t *canvas_wrap_fb(void)
{
    if (!fb_available()) return NULL;

    uint32_t w = fb_get_width();
    uint32_t h = fb_get_height();

    canvas_t *c = (canvas_t *)kmalloc(sizeof(canvas_t));
    if (!c) return NULL;

    c->width  = w;
    c->height = h;
    c->pitch  = w;
    c->is_external = false;
    c->pixels = (color_t *)kmalloc(w * h * sizeof(color_t));
    if (!c->pixels) {
        kfree(c);
        return NULL;
    }
    canvas_clear(c, COLOR_BLACK);
    return c;
}

void canvas_present(canvas_t *c)
{
    if (!c || !c->pixels || !fb_available()) return;

    uint32_t fw      = fb_get_width();
    uint32_t fh      = fb_get_height();
    uint32_t fb_bpp  = fb_get_bpp_bytes();
    uint32_t fb_pitch = fb_get_pitch();
    uint8_t *fb_addr = fb_get_addr();

    uint32_t copy_w = c->width  < fw ? c->width  : fw;
    uint32_t copy_h = c->height < fh ? c->height : fh;

    for (uint32_t y = 0; y < copy_h; y++) {
        color_t *src_row = c->pixels + y * c->pitch;

        if (fb_bpp == 4) {
            uint32_t *dst_row = (uint32_t *)(fb_addr + y * fb_pitch);
            for (uint32_t x = 0; x < copy_w; x++) {
                color_t clr = src_row[x];
                dst_row[x] = (clr & 0xFF) | ((clr >> 8) & 0xFF) << 8 |
                             ((clr >> 16) & 0xFF) << 16 | ((clr >> 24) & 0xFF) << 24;
            }
        } else if (fb_bpp == 3) {
            uint8_t *dst_row = fb_addr + y * fb_pitch;
            for (uint32_t x = 0; x < copy_w; x++) {
                color_t clr = src_row[x];
                dst_row[x * 3]     = (uint8_t)(clr & 0xFF);
                dst_row[x * 3 + 1] = (uint8_t)((clr >> 8) & 0xFF);
                dst_row[x * 3 + 2] = (uint8_t)((clr >> 16) & 0xFF);
            }
        } else if (fb_bpp == 2) {
            uint16_t *dst_row = (uint16_t *)(fb_addr + y * fb_pitch);
            for (uint32_t x = 0; x < copy_w; x++) {
                color_t clr = src_row[x];
                dst_row[x] = (uint16_t)((clr & 0xFF) | ((clr >> 8) & 0xFF) << 8);
            }
        } else {
            for (uint32_t x = 0; x < copy_w; x++) {
                fb_putpixel(x, y, src_row[x]);
            }
        }
    }
}