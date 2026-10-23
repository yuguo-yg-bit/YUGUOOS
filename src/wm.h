#pragma once
#include "win.h"
#include "input.h"

void   wm_init(void);
win_t *wm_create_window(const char *title, int x, int y, int w, int h);
void   wm_destroy_window(win_t *win);
void   wm_focus_window(win_t *win);
void   wm_raise_window(win_t *win);

void   wm_composite(void);
void   wm_present(void);
void   wm_handle_input(input_event_t *ev);
void   wm_handle_mouse(int x, int y, bool left_press, bool left_release);
void   wm_redraw_all(void);

typedef void (*wm_desktop_draw_fn)(canvas_t *c);
void   wm_set_desktop_draw(wm_desktop_draw_fn fn);

win_t *wm_get_focused(void);
int    wm_window_count(void);
bool   wm_point_in_window(int x, int y);