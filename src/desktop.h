#pragma once
#include "canvas.h"

typedef void (*desktop_icon_dclick_fn)(void);

void desktop_init(void);
int  desktop_add_icon(int x, int y, int w, int h, const char *name, desktop_icon_dclick_fn fn);
void desktop_draw(canvas_t *c);
bool desktop_handle_click(int x, int y);
void desktop_destroy(void);