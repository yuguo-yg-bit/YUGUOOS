#include "gfx.h"

void gfx_draw_bevel_rect(canvas_t *c, int x, int y, int w, int h,
                         color_t highlight, color_t shadow)
{
    if (!c || w <= 0 || h <= 0) return;
    canvas_draw_hline(c, x, y, w, highlight);
    canvas_draw_vline(c, x, y, h, highlight);
    canvas_draw_hline(c, x, y + h - 1, w, shadow);
    canvas_draw_vline(c, x + w - 1, y, h, shadow);
}

void gfx_draw_sunken_rect(canvas_t *c, int x, int y, int w, int h,
                          color_t dark, color_t light)
{
    if (!c || w <= 0 || h <= 0) return;
    canvas_draw_hline(c, x, y, w, dark);
    canvas_draw_vline(c, x, y, h, dark);
    canvas_draw_hline(c, x, y + h - 1, w, light);
    canvas_draw_vline(c, x + w - 1, y, h, light);
}

void gfx_draw_title_bar(canvas_t *c, int x, int y, int w,
                        color_t bg, const char *title, color_t text_color)
{
    if (!c || w <= 0) return;

    int bar_h = 24;

    canvas_fill_rect(c, x, y, w, bar_h, bg);

    canvas_draw_hline(c, x, y, w, COLOR_WHITE);

    if (title && title[0]) {
        int title_x = x + 6;
        int title_y = y + 5;
        canvas_draw_string(c, title_x, title_y, title, text_color, bg);
    }
}

void gfx_draw_close_button(canvas_t *c, int x, int y, int size, bool hover)
{
    if (!c || size < 8) return;

    color_t face = hover ? COLOR_RED : COLOR_BUTTON_FACE;
    color_t highlight = hover ? COLOR_RED : COLOR_BUTTON_HIGHLIGHT;
    color_t shadow = hover ? COLOR_DARK_RED : COLOR_BUTTON_SHADOW;
    color_t x_color = hover ? COLOR_WHITE : COLOR_BLACK;

    canvas_fill_rect(c, x, y, size, size, face);
    gfx_draw_bevel_rect(c, x, y, size, size, highlight, shadow);

    int inner = size / 3;
    int x0 = x + inner;
    int y0 = y + inner;
    int x1 = x + size - inner - 1;
    int y1 = y + size - inner - 1;

    for (int i = -1; i <= 1; i++) {
        int px = x0, py = y0 + i;
        while (px <= x1 && py <= y1) {
            canvas_putpixel(c, px, py, x_color);
            px++;
            py++;
        }
    }

    for (int i = -1; i <= 1; i++) {
        int px = x0 + i, py = y1;
        while (px <= x1 && py >= y0) {
            canvas_putpixel(c, px, py, x_color);
            px++;
            py--;
        }
    }
}

void gfx_draw_button(canvas_t *c, int x, int y, int w, int h,
                     const char *label, bool pressed)
{
    if (!c || w < 8 || h < 8) return;

    if (pressed) {
        canvas_fill_rect(c, x, y, w, h, COLOR_BUTTON_FACE);
        gfx_draw_sunken_rect(c, x, y, w, h, COLOR_BUTTON_SHADOW, COLOR_BUTTON_HIGHLIGHT);
    } else {
        canvas_fill_rect(c, x, y, w, h, COLOR_BUTTON_FACE);
        gfx_draw_bevel_rect(c, x, y, w, h, COLOR_BUTTON_HIGHLIGHT, COLOR_BUTTON_SHADOW);
    }

    if (label && label[0]) {
        int str_len = 0;
        const char *p = label;
        while (*p) { str_len++; p++; }

        int label_x = x + (w - str_len * CANVAS_CHAR_W) / 2;
        int label_y = y + (h - CANVAS_CHAR_H) / 2;
        int offset = pressed ? 1 : 0;
        canvas_draw_string(c, label_x + offset, label_y + offset, label, COLOR_BLACK, COLOR_BUTTON_FACE);
    }
}

void gfx_draw_window_frame(canvas_t *c, int x, int y, int w, int h, bool focused)
{
    if (!c || w < 40 || h < 40) return;

    color_t title_bg = focused ? COLOR_TITLE_BAR : COLOR_TITLE_BAR_INACTIVE;

    canvas_fill_rect(c, x, y, w, h, COLOR_WINDOW_BG);

    gfx_draw_bevel_rect(c, x, y, w, h, COLOR_WHITE, COLOR_BLACK);
    gfx_draw_bevel_rect(c, x + 1, y + 1, w - 2, h - 2, COLOR_LIGHT_GREY, COLOR_DARK_GREY);

    canvas_fill_rect(c, x + 2, y + 2, w - 4, 22, title_bg);
}