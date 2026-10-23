#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    int      x, y;
    bool     left;
    bool     right;
    bool     middle;
    bool     valid;
} mouse_state_t;

void  mouse_init(void);
void  mouse_poll(void);
void  mouse_get_state(mouse_state_t *state);
void  mouse_draw_cursor(void);
void  mouse_clear_cursor(void);
void  mouse_update_and_draw(void);
void  mouse_draw_at(void);

extern mouse_state_t g_mouse;