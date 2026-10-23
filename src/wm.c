#include "wm.h"
#include "widget.h"
#include "gfx.h"
#include "fb.h"
#include "serial.h"
#include "vmm.h"

static canvas_t *screen_back = NULL;
static win_t    *window_list_head = NULL;
static win_t    *window_list_tail = NULL;
static win_t    *focused_win = NULL;
static int       window_count = 0;

void wm_init(void)
{
    if (fb_available()) {
        screen_back = canvas_wrap_fb();
    } else {
        screen_back = NULL;
    }
    window_list_head = NULL;
    window_list_tail = NULL;
    focused_win = NULL;
    window_count = 0;

    canvas_clear(screen_back, COLOR_DESKTOP_BG);
}

static void wm_insert_window(win_t *win)
{
    if (!win) return;

    win->next = NULL;
    win->prev = window_list_tail;

    if (window_list_tail) {
        window_list_tail->next = win;
    } else {
        window_list_head = win;
    }
    window_list_tail = win;
    window_count++;
}

static void wm_remove_from_list(win_t *win)
{
    if (!win) return;

    if (win->prev) {
        win->prev->next = win->next;
    } else {
        window_list_head = win->next;
    }

    if (win->next) {
        win->next->prev = win->prev;
    } else {
        window_list_tail = win->prev;
    }

    if (focused_win == win) {
        focused_win = window_list_tail;
        if (focused_win) {
            focused_win->focused = true;
            focused_win->dirty = true;
        }
    }

    win->next = NULL;
    win->prev = NULL;
    window_count--;
}

win_t *wm_create_window(const char *title, int x, int y, int w, int h)
{
    win_t *win = win_create(title, x, y, w, h);
    if (!win) return NULL;

    wm_insert_window(win);
    wm_focus_window(win);
    wm_redraw_all();

    return win;
}

void wm_destroy_window(win_t *win)
{
    if (!win) return;

    wm_remove_from_list(win);

    if (win->cleanup) {
        win->cleanup(win);
    }
    if (win->widgets) {
        widget_mgr_destroy(win->widgets);
        win->widgets = NULL;
    }

    win_destroy(win);
    wm_redraw_all();
}

void wm_focus_window(win_t *win)
{
    if (!win) return;

    if (focused_win && focused_win != win) {
        focused_win->focused = false;
        focused_win->dirty = true;
    }

    focused_win = win;
    win->focused = true;
    win->dirty = true;
}

void wm_raise_window(win_t *win)
{
    if (!win || window_list_tail == win) return;

    wm_remove_from_list(win);
    wm_insert_window(win);
    wm_focus_window(win);
}

void wm_composite(void)
{
    if (!screen_back) return;

    canvas_clear(screen_back, COLOR_DESKTOP_BG);

    win_t *win = window_list_head;
    while (win) {
        if (win->visible) {
            win_draw_self(win);

            canvas_blit(screen_back, win->rect.x, win->rect.y,
                        win->canvas, 0, 0,
                        win->rect.w, win->rect.h);
        }
        win = win->next;
    }
}

void wm_present(void)
{
    if (screen_back) {
        canvas_present(screen_back);
    }
}

void wm_handle_input(input_event_t *ev)
{
    if (!ev) return;

    char c = ev->key;

    widget_mgr_t *w_mgr = NULL;
    bool textbox_active = false;
    if (focused_win && focused_win->widgets) {
        w_mgr = focused_win->widgets;
        textbox_active = widget_mgr_has_textbox_focus(w_mgr);
    }

    if (!textbox_active) {
        switch (c) {
        case '\t':
            if (focused_win) {
                win_t *target = focused_win->next;
                if (!target) target = window_list_head;
                if (target) {
                    wm_focus_window(target);
                    wm_redraw_all();
                }
            } else if (window_list_tail) {
                wm_focus_window(window_list_tail);
                wm_redraw_all();
            }
            return;

        case 'q':
            if (focused_win) {
                wm_destroy_window(focused_win);
            }
            return;

        case 'w':
            if (focused_win) {
                focused_win->rect.y -= 20;
                focused_win->dirty = true;
                wm_redraw_all();
            }
            return;

        case 'a':
            if (focused_win) {
                focused_win->rect.x -= 20;
                focused_win->dirty = true;
                wm_redraw_all();
            }
            return;

        case 's':
            if (focused_win) {
                focused_win->rect.y += 20;
                focused_win->dirty = true;
                wm_redraw_all();
            }
            return;

        case 'd':
            if (focused_win) {
                focused_win->rect.x += 20;
                focused_win->dirty = true;
                wm_redraw_all();
            }
            return;

        case 'r':
            if (focused_win) {
                wm_raise_window(focused_win);
                wm_redraw_all();
            }
            return;

        default:
            break;
        }
    }

    if (focused_win) {
        if (w_mgr) {
            if (widget_mgr_handle_key(w_mgr, c)) {
                focused_win->dirty = true;
                wm_redraw_all();
            }
        } else {
            win_content_putchar(focused_win, c);
            if (focused_win->dirty) {
                wm_redraw_all();
            }
        }
    }
}

void wm_handle_mouse(int x, int y, bool left_press, bool left_release)
{
    win_t *win;
    win_t *hit_win = NULL;

    for (win = window_list_tail; win; win = win->prev) {
        if (win->visible && win_hit_test(win, x, y)) {
            hit_win = win;
            break;
        }
    }

    if (left_press) {
        if (hit_win) {
            wm_focus_window(hit_win);

            if (win_hit_close(hit_win, x, y)) {
                wm_destroy_window(hit_win);
                return;
            }

            widget_mgr_t *w_mgr = hit_win->widgets;
            if (w_mgr) {
                int wx = x - hit_win->rect.x - WIDGET_OX;
                int wy = y - hit_win->rect.y - WIDGET_OY;
                widget_mgr_handle_click(w_mgr, wx, wy);
            }

            wm_redraw_all();
        }
    }

    if (left_release) {
        (void)0;
    }
}

void wm_redraw_all(void)
{
    win_t *win = window_list_head;
    while (win) {
        win->dirty = true;
        win = win->next;
    }
    wm_composite();
}

win_t *wm_get_focused(void)
{
    return focused_win;
}

int wm_window_count(void)
{
    return window_count;
}