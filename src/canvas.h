#pragma once
#include "types.h"

typedef struct {
    color_t  *pixels;
    uint32_t  width;
    uint32_t  height;
    uint32_t  pitch;
    bool      is_external;
} canvas_t;

canvas_t *canvas_create(uint32_t w, uint32_t h);
void      canvas_destroy(canvas_t *c);
void      canvas_clear(canvas_t *c, color_t color);

void      canvas_putpixel(canvas_t *c, int x, int y, color_t color);
color_t   canvas_getpixel(canvas_t *c, int x, int y);

void      canvas_fill_rect(canvas_t *c, int x, int y, int w, int h, color_t color);
void      canvas_draw_rect(canvas_t *c, int x, int y, int w, int h, color_t color);
void      canvas_draw_hline(canvas_t *c, int x, int y, int len, color_t color);
void      canvas_draw_vline(canvas_t *c, int x, int y, int len, color_t color);

void      canvas_blit(canvas_t *dst, int dx, int dy, canvas_t *src, int sx, int sy, int w, int h);

void      canvas_draw_char(canvas_t *c, int x, int y, char ch, color_t fg, color_t bg);
void      canvas_draw_string(canvas_t *c, int x, int y, const char *str, color_t fg, color_t bg);

canvas_t *canvas_wrap_fb(void);
void      canvas_present(canvas_t *c);

#define CANVAS_CHAR_W 8
#define CANVAS_CHAR_H 16