#pragma once
#include "canvas.h"

void gfx_draw_bevel_rect(canvas_t *c, int x, int y, int w, int h,
                         color_t top_left, color_t bottom_right);

void gfx_draw_sunken_rect(canvas_t *c, int x, int y, int w, int h,
                          color_t top_left, color_t bottom_right);

void gfx_draw_title_bar(canvas_t *c, int x, int y, int w,
                        color_t bg, const char *title, color_t text_color);

void gfx_draw_close_button(canvas_t *c, int x, int y, int size, bool hover);

void gfx_draw_button(canvas_t *c, int x, int y, int w, int h,
                     const char *label, bool pressed);

void gfx_draw_window_frame(canvas_t *c, int x, int y, int w, int h,
                           bool focused);

void gfx_draw_desktop_bg(canvas_t *c);