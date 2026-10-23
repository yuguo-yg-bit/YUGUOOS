#include "win.h"
#include "gfx.h"
#include "vmm.h"

static uint32_t next_win_id = 100;

int win_titlebar_height(void) { return 24; }
int win_border_width(void)   { return 2; }
int win_content_x(void)      { return 4; }
int win_content_y(void)      { return 28; }
int win_content_w(win_t *win) { return win ? win->rect.w - 8 : 0; }
int win_content_h(win_t *win) { return win ? win->rect.h - 32 : 0; }

bool win_hit_test(win_t *win, int x, int y)
{
    if (!win) return false;
    return (x >= win->rect.x && x < win->rect.x + win->rect.w &&
            y >= win->rect.y && y < win->rect.y + win->rect.h);
}

bool win_hit_close(win_t *win, int x, int y)
{
    if (!win) return false;
    int local_x = x - win->rect.x;
    int local_y = y - win->rect.y;
    int btn_size = 18;
    int btn_x = win->rect.w - 2 - 2 - btn_size;
    int btn_y = 2 + 3;
    return (local_x >= btn_x && local_x < btn_x + btn_size &&
            local_y >= btn_y && local_y < btn_y + btn_size);
}

bool win_hit_titlebar(win_t *win, int x, int y)
{
    if (!win) return false;
    int local_x = x - win->rect.x;
    int local_y = y - win->rect.y;
    return (local_x >= 4 && local_x < win->rect.w - 4 &&
            local_y >= 2 && local_y < 2 + 24);
}

win_t *win_create(const char *title, int x, int y, int w, int h)
{
    win_t *win = (win_t *)kmalloc(sizeof(win_t));
    if (!win) return NULL;

    win->id      = next_win_id++;
    win->rect.x  = x;
    win->rect.y  = y;
    win->rect.w  = w;
    win->rect.h  = h;
    win->z_order = 0;
    win->visible = true;
    win->focused = false;
    win->dirty   = true;
    win->next    = NULL;
    win->prev    = NULL;
    win->draw_content = NULL;
    win->input_char   = NULL;
    win->user_data    = NULL;
    win->content_pos  = 0;

    int i;
    for (i = 0; i < WIN_MAX_TITLE - 1 && title && title[i]; i++) {
        win->title[i] = title[i];
    }
    win->title[i] = '\0';

    for (int j = 0; j < WIN_CONTENT_BUF_SIZE; j++) {
        win->content_buf[j] = '\0';
    }

    win->canvas = canvas_create((uint32_t)w, (uint32_t)h);
    if (!win->canvas) {
        kfree(win);
        return NULL;
    }

    return win;
}

void win_destroy(win_t *win)
{
    if (!win) return;
    if (win->canvas) {
        canvas_destroy(win->canvas);
        win->canvas = NULL;
    }
    kfree(win);
}

void win_move(win_t *win, int x, int y)
{
    if (!win) return;
    win->rect.x = x;
    win->rect.y = y;
    win->dirty = true;
}

void win_resize(win_t *win, int w, int h)
{
    if (!win || w <= 0 || h <= 0) return;

    canvas_t *new_canvas = canvas_create((uint32_t)w, (uint32_t)h);
    if (!new_canvas) return;

    canvas_blit(new_canvas, 0, 0, win->canvas, 0, 0,
                w < win->rect.w ? w : win->rect.w,
                h < win->rect.h ? h : win->rect.h);

    canvas_destroy(win->canvas);
    win->canvas = new_canvas;
    win->rect.w = w;
    win->rect.h = h;
    win->dirty = true;
}

void win_set_title(win_t *win, const char *title)
{
    if (!win) return;
    int i;
    for (i = 0; i < WIN_MAX_TITLE - 1 && title && title[i]; i++) {
        win->title[i] = title[i];
    }
    win->title[i] = '\0';
    win->dirty = true;
}

void win_draw_self(win_t *win)
{
    if (!win || !win->canvas) return;
    if (!win->dirty) return;

    canvas_t *c = win->canvas;
    int w = win->rect.w;
    int h = win->rect.h;

    canvas_fill_rect(c, 0, 0, w, h, COLOR_WINDOW_BG);

    gfx_draw_bevel_rect(c, 0, 0, w, h, COLOR_WHITE, COLOR_BLACK);
    gfx_draw_bevel_rect(c, 1, 1, w - 2, h - 2, COLOR_LIGHT_GREY, COLOR_DARK_GREY);

    color_t title_bg = win->focused ? COLOR_TITLE_BAR : COLOR_TITLE_BAR_INACTIVE;
    gfx_draw_title_bar(c, 2, 2, w - 4, title_bg, win->title, COLOR_WHITE);

    int btn_size = 18;
    int btn_x = w - 2 - 2 - btn_size;
    int btn_y = 2 + 3;
    gfx_draw_close_button(c, btn_x, btn_y, btn_size, false);

    if (win->draw_content) {
        win->draw_content(win);
    }

    win->dirty = false;
}

void win_mark_dirty(win_t *win)
{
    if (win) win->dirty = true;
}

void win_content_putchar(win_t *win, char c)
{
    if (!win) return;

    if (c == '\b') {
        if (win->content_pos > 0) {
            win->content_pos--;
            win->content_buf[win->content_pos] = '\0';
        }
    } else if (c == '\n' || c == '\r') {
        if (win->input_char) {
            win->input_char(win, c);
        }
        win->content_pos = 0;
        win->content_buf[0] = '\0';
        win->dirty = true;
    } else if (c >= 32 && c <= 126) {
        if (win->content_pos < WIN_CONTENT_BUF_SIZE - 1) {
            win->content_buf[win->content_pos++] = c;
            win->content_buf[win->content_pos] = '\0';
            win->dirty = true;
        }
    }
}

void win_content_clear(win_t *win)
{
    if (!win) return;
    win->content_pos = 0;
    win->content_buf[0] = '\0';
    win->dirty = true;
}

void win_content_draw_text(win_t *win, int x, int y, const char *str, color_t fg)
{
    if (!win || !win->canvas || !str) return;
    canvas_draw_string(win->canvas, win_content_x() + x, win_content_y() + y,
                       str, fg, COLOR_WINDOW_BG);
}