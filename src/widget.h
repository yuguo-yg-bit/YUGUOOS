#pragma once
#include "types.h"
#include "canvas.h"

typedef struct widget widget_t;
typedef struct widget_mgr widget_mgr_t;

typedef void (*widget_callback_t)(widget_t *w);

typedef enum {
    WIDGET_LABEL = 0,
    WIDGET_BUTTON,
    WIDGET_TEXTBOX,
    WIDGET_CHECKBOX,
    WIDGET_GROUPBOX,
} widget_type_t;

struct widget {
    widget_type_t type;
    rect_t        rect;
    bool          visible;
    bool          enabled;
    int           id;

    char     text[256];
    bool     pressed;
    bool     checked;
    int      cursor_pos;
    int      scroll_offset;
    bool     has_focus;

    color_t  fg;
    color_t  bg;

    widget_callback_t on_click;
    widget_callback_t on_change;
    void             *user_data;

    widget_t *next;
    widget_t *prev;
};

struct widget_mgr {
    widget_t *head;
    widget_t *tail;
    widget_t *focus_widget;
    int       count;
};

#define WIDGET_OX 4
#define WIDGET_OY 28

widget_mgr_t *widget_mgr_create(void);
void          widget_mgr_destroy(widget_mgr_t *mgr);

widget_t *widget_create_label(widget_mgr_t *mgr, int x, int y, const char *text);
widget_t *widget_create_button(widget_mgr_t *mgr, int x, int y, int w, int h,
                                const char *text, widget_callback_t on_click);
widget_t *widget_create_textbox(widget_mgr_t *mgr, int x, int y, int w,
                                 const char *text, widget_callback_t on_change);
widget_t *widget_create_checkbox(widget_mgr_t *mgr, int x, int y,
                                  const char *text, bool checked, widget_callback_t on_change);
widget_t *widget_create_groupbox(widget_mgr_t *mgr, int x, int y, int w, int h, const char *title);

void  widget_mgr_draw_all(widget_mgr_t *mgr, canvas_t *canvas);
bool  widget_mgr_handle_click(widget_mgr_t *mgr, int x, int y);
bool  widget_mgr_handle_key(widget_mgr_t *mgr, char c);
bool  widget_mgr_has_textbox_focus(widget_mgr_t *mgr);
widget_t *widget_mgr_find_at(widget_mgr_t *mgr, int x, int y);

void        widget_set_text(widget_t *w, const char *text);
const char *widget_get_text(widget_t *w);
void        widget_set_enabled(widget_t *w, bool enabled);
void        widget_set_fg(widget_t *w, color_t color);