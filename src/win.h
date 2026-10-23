#pragma once
#include "canvas.h"

#define WIN_MAX_TITLE 64
#define WIN_CONTENT_BUF_SIZE 512

typedef struct win win_t;
typedef struct widget_mgr widget_mgr_t;

typedef void (*win_draw_fn_t)(win_t *win);
typedef void (*win_input_fn_t)(win_t *win, char c);
typedef void (*win_cleanup_fn_t)(win_t *win);

struct win {
    uint32_t    id;
    char        title[WIN_MAX_TITLE];
    rect_t      rect;
    canvas_t   *canvas;
    int         z_order;
    bool        visible;
    bool        focused;
    bool        dirty;
    struct win *next;
    struct win *prev;

    win_draw_fn_t    draw_content;
    win_input_fn_t   input_char;
    win_cleanup_fn_t cleanup;
    widget_mgr_t    *widgets;
    void            *user_data;

    char        content_buf[WIN_CONTENT_BUF_SIZE];
    int         content_pos;
};

win_t *win_create(const char *title, int x, int y, int w, int h);
void   win_destroy(win_t *win);
void   win_move(win_t *win, int x, int y);
void   win_resize(win_t *win, int w, int h);
void   win_set_title(win_t *win, const char *title);
void   win_draw_self(win_t *win);
void   win_mark_dirty(win_t *win);

void   win_content_putchar(win_t *win, char c);
void   win_content_clear(win_t *win);
void   win_content_draw_text(win_t *win, int x, int y, const char *str, color_t fg);

int win_titlebar_height(void);
int win_border_width(void);
int win_content_x(void);
int win_content_y(void);
int win_content_w(win_t *win);
int win_content_h(win_t *win);
bool win_hit_test(win_t *win, int x, int y);
bool win_hit_close(win_t *win, int x, int y);
bool win_hit_titlebar(win_t *win, int x, int y);