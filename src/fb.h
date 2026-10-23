#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define FB_COLOR(r, g, b) (((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | (uint32_t)(b))

#define FB_BLACK       0x00000000
#define FB_WHITE       0x00FFFFFF
#define FB_RED         0x00FF0000
#define FB_GREEN       0x0000FF00
#define FB_BLUE        0x000000FF
#define FB_CYAN        0x0000FFFF
#define FB_MAGENTA     0x00FF00FF
#define FB_YELLOW      0x00FFFF00
#define FB_DARK_GREY   0x00333333
#define FB_MID_GREY    0x00666666
#define FB_LIGHT_GREY  0x00AAAAAA
#define FB_ORANGE      0x00FF8800
#define FB_DARK_BLUE   0x00000088
#define FB_DARK_RED    0x00880000
#define FB_DARK_GREEN  0x00008800

struct fb_info {
    uint8_t *addr;
    uint32_t pitch;
    uint32_t width;
    uint32_t height;
    uint32_t bpp;
    uint32_t type;
    uint8_t  bpp_bytes;
    uint8_t  red_pos;
    uint8_t  red_mask;
    uint8_t  green_pos;
    uint8_t  green_mask;
    uint8_t  blue_pos;
    uint8_t  blue_mask;
};

void   fb_init(uintptr_t multiboot_addr);
bool   fb_available(void);
void   fb_clear(uint32_t color);
void   fb_putpixel(uint32_t x, uint32_t y, uint32_t color);
uint32_t fb_getpixel(uint32_t x, uint32_t y);

void fb_draw_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);
void fb_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);
void fb_draw_char(uint32_t x, uint32_t y, char c, uint32_t fg, uint32_t bg);
void fb_draw_string(uint32_t x, uint32_t y, const char *str, uint32_t fg, uint32_t bg);
void fb_draw_string_centered(uint32_t y, const char *str, uint32_t fg, uint32_t bg);
uint32_t fb_get_width(void);
uint32_t fb_get_height(void);

void fb_draw_big_char(uint32_t x, uint32_t y, char c, uint32_t fg);
void fb_draw_big_string(uint32_t x, uint32_t y, const char *str, uint32_t fg);
void fb_draw_big_string_centered(uint32_t y, const char *str, uint32_t fg);

void fb_draw_huge_char(uint32_t x, uint32_t y, char c, uint32_t fg);
void fb_draw_huge_string(uint32_t x, uint32_t y, const char *str, uint32_t fg);
void fb_draw_huge_string_centered(uint32_t y, const char *str, uint32_t fg);

#define FB_CHAR_W 8
#define FB_CHAR_H 16
#define FB_BIG_CHAR_W 16
#define FB_BIG_CHAR_H 32
#define FB_HUGE_CHAR_W 32
#define FB_HUGE_CHAR_H 64