#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    int x, y;
} point_t;

typedef struct {
    int x, y;
    int w, h;
} rect_t;

typedef uint32_t color_t;

#define COLOR_BLACK       0x00000000
#define COLOR_WHITE       0x00FFFFFF
#define COLOR_RED         0x00FF0000
#define COLOR_GREEN       0x0000FF00
#define COLOR_BLUE        0x000000FF
#define COLOR_CYAN        0x0000FFFF
#define COLOR_MAGENTA     0x00FF00FF
#define COLOR_YELLOW      0x00FFFF00
#define COLOR_ORANGE      0x00FF8800

#define COLOR_DARK_GREY   0x00333333
#define COLOR_MID_GREY    0x00666666
#define COLOR_LIGHT_GREY  0x00AAAAAA
#define COLOR_SILVER      0x00C0C0C0
#define COLOR_GAINSBORO   0x00DCDCDC

#define COLOR_DARK_BLUE   0x00000088
#define COLOR_DARK_RED    0x00880000
#define COLOR_DARK_GREEN  0x00008800
#define COLOR_NAVY        0x00000080
#define COLOR_TEAL        0x00008080

#define COLOR_TITLE_BAR     0x000050A0
#define COLOR_TITLE_BAR_INACTIVE 0x00606060
#define COLOR_WINDOW_BG     0x00F0F0F0
#define COLOR_DESKTOP_BG    0x00003060
#define COLOR_BUTTON_FACE   0x00D0D0D0
#define COLOR_BUTTON_SHADOW 0x00808080
#define COLOR_BUTTON_HIGHLIGHT 0x00FFFFFF

#define COLOR_TEXT      0x00000000
#define COLOR_HIGHLIGHT 0x00000080

#define COLOR_RGB(r, g, b) (((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | (uint32_t)(b))