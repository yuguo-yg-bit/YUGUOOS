#include "widget.h"
#include "gfx.h"
#include "vmm.h"
#include "serial.h"

static int widget_strlen(const char *s)
{
    int len = 0;
    if (!s) return 0;
    while (s[len]) len++;
    return len;
}

static void widget_strcpy(char *dst, const char *src, int max)
{
    int i;
    for (i = 0; i < max - 1 && src && src[i]; i++) {
        dst[i] = src[i];
    }
    dst[i] = '\0';
}

static int next_widget_id = 1000;

widget_mgr_t *widget_mgr_create(void)
{
    widget_mgr_t *mgr = (widget_mgr_t *)kmalloc(sizeof(widget_mgr_t));
    if (!mgr) return NULL;

    mgr->head = NULL;
    mgr->tail = NULL;
    mgr->focus_widget = NULL;
    mgr->count = 0;

    serial_write("[WIDGET] 控件管理器已创建\n");
    return mgr;
}

void widget_mgr_destroy(widget_mgr_t *mgr)
{
    if (!mgr) return;

    widget_t *w = mgr->head;
    while (w) {
        widget_t *next = w->next;
        kfree(w);
        w = next;
    }

    kfree(mgr);
    serial_write("[WIDGET] 控件管理器已销毁\n");
}

static void widget_mgr_insert(widget_mgr_t *mgr, widget_t *w)
{
    if (!mgr || !w) return;

    w->next = NULL;
    w->prev = mgr->tail;

    if (mgr->tail) {
        mgr->tail->next = w;
    } else {
        mgr->head = w;
    }
    mgr->tail = w;
    mgr->count++;
}

static widget_t *widget_alloc(widget_mgr_t *mgr, widget_type_t type,
                               int x, int y, int w, int h)
{
    widget_t *widget = (widget_t *)kmalloc(sizeof(widget_t));
    if (!widget) return NULL;

    widget->type = type;
    widget->rect.x = x;
    widget->rect.y = y;
    widget->rect.w = w;
    widget->rect.h = h;
    widget->visible = true;
    widget->enabled = true;
    widget->id = next_widget_id++;

    int i;
    for (i = 0; i < 256; i++) widget->text[i] = '\0';
    widget->pressed = false;
    widget->checked = false;
    widget->cursor_pos = 0;
    widget->scroll_offset = 0;
    widget->has_focus = false;

    widget->fg = COLOR_TEXT;
    widget->bg = COLOR_WINDOW_BG;

    widget->on_click = NULL;
    widget->on_change = NULL;
    widget->user_data = NULL;

    widget->next = NULL;
    widget->prev = NULL;

    widget_mgr_insert(mgr, widget);
    return widget;
}

widget_t *widget_create_label(widget_mgr_t *mgr, int x, int y, const char *text)
{
    int text_len = widget_strlen(text);
    int w = text_len * CANVAS_CHAR_W;
    int h = CANVAS_CHAR_H;

    widget_t *widget = widget_alloc(mgr, WIDGET_LABEL, x, y, w, h);
    if (!widget) return NULL;

    if (text) widget_strcpy(widget->text, text, 256);
    widget->fg = COLOR_TEXT;

    serial_write("[WIDGET] 创建标签: ");
    serial_write(text ? text : "");
    serial_write("\n");
    return widget;
}

widget_t *widget_create_button(widget_mgr_t *mgr, int x, int y, int w, int h,
                                const char *text, widget_callback_t on_click)
{
    widget_t *widget = widget_alloc(mgr, WIDGET_BUTTON, x, y, w, h);
    if (!widget) return NULL;

    if (text) widget_strcpy(widget->text, text, 256);
    widget->on_click = on_click;
    widget->pressed = false;

    serial_write("[WIDGET] 创建按钮: ");
    serial_write(text ? text : "");
    serial_write("\n");
    return widget;
}

widget_t *widget_create_textbox(widget_mgr_t *mgr, int x, int y, int w,
                                 const char *text, widget_callback_t on_change)
{
    int h = CANVAS_CHAR_H + 6;

    widget_t *widget = widget_alloc(mgr, WIDGET_TEXTBOX, x, y, w, h);
    if (!widget) return NULL;

    if (text) {
        widget_strcpy(widget->text, text, 256);
        widget->cursor_pos = widget_strlen(text);
    }
    widget->on_change = on_change;

    serial_write("[WIDGET] 创建文本框\n");
    return widget;
}

widget_t *widget_create_checkbox(widget_mgr_t *mgr, int x, int y,
                                  const char *text, bool checked, widget_callback_t on_change)
{
    int box_size = 14;
    int text_len = widget_strlen(text);
    int w = box_size + 6 + text_len * CANVAS_CHAR_W;
    int h = box_size > CANVAS_CHAR_H ? box_size : CANVAS_CHAR_H;

    widget_t *widget = widget_alloc(mgr, WIDGET_CHECKBOX, x, y, w, h);
    if (!widget) return NULL;

    if (text) widget_strcpy(widget->text, text, 256);
    widget->checked = checked;
    widget->on_change = on_change;

    serial_write("[WIDGET] 创建复选框: ");
    serial_write(text ? text : "");
    serial_write("\n");
    return widget;
}

widget_t *widget_create_groupbox(widget_mgr_t *mgr, int x, int y, int w, int h,
                                  const char *title)
{
    widget_t *widget = widget_alloc(mgr, WIDGET_GROUPBOX, x, y, w, h);
    if (!widget) return NULL;

    if (title) widget_strcpy(widget->text, title, 256);

    serial_write("[WIDGET] 创建分组框: ");
    serial_write(title ? title : "");
    serial_write("\n");
    return widget;
}

void widget_set_text(widget_t *w, const char *text)
{
    if (!w) return;
    widget_strcpy(w->text, text, 256);
    if (w->type == WIDGET_TEXTBOX) {
        w->cursor_pos = widget_strlen(w->text);
    }
}

const char *widget_get_text(widget_t *w)
{
    if (!w) return "";
    return w->text;
}

void widget_set_enabled(widget_t *w, bool enabled)
{
    if (!w) return;
    w->enabled = enabled;
}

void widget_set_fg(widget_t *w, color_t color)
{
    if (!w) return;
    w->fg = color;
}

static void widget_draw_label(widget_t *w, canvas_t *canvas, int ox, int oy)
{
    int x = w->rect.x + ox;
    int y = w->rect.y + oy;

    color_t fg = w->enabled ? w->fg : COLOR_MID_GREY;
    canvas_draw_string(canvas, x, y, w->text, fg, COLOR_WINDOW_BG);
}

static void widget_draw_button(widget_t *w, canvas_t *canvas, int ox, int oy)
{
    int x = w->rect.x + ox;
    int y = w->rect.y + oy;

    gfx_draw_button(canvas, x, y, w->rect.w, w->rect.h, w->text, w->pressed);

    if (!w->enabled) {
        int i, j;
        for (j = 0; j < w->rect.h; j += 2) {
            for (i = 0; i < w->rect.w; i += 2) {
                canvas_putpixel(canvas, x + i, y + j, COLOR_MID_GREY);
            }
        }
    }
}

static void widget_draw_textbox(widget_t *w, canvas_t *canvas, int ox, int oy)
{
    int x = w->rect.x + ox;
    int y = w->rect.y + oy;
    int ww = w->rect.w;
    int wh = w->rect.h;

    canvas_fill_rect(canvas, x, y, ww, wh, COLOR_WHITE);
    gfx_draw_sunken_rect(canvas, x, y, ww, wh, COLOR_BUTTON_SHADOW, COLOR_BUTTON_HIGHLIGHT);

    int text_len = widget_strlen(w->text);
    int padding = 4;

    int visible_chars = (ww - 2 * padding) / CANVAS_CHAR_W;
    if (visible_chars < 1) visible_chars = 1;

    if (w->cursor_pos < w->scroll_offset) {
        w->scroll_offset = w->cursor_pos;
    }
    if (w->cursor_pos >= w->scroll_offset + visible_chars) {
        w->scroll_offset = w->cursor_pos - visible_chars + 1;
    }
    if (w->scroll_offset < 0) w->scroll_offset = 0;

    int draw_len = text_len - w->scroll_offset;
    if (draw_len > visible_chars) draw_len = visible_chars;
    if (draw_len < 0) draw_len = 0;

    if (draw_len > 0) {
        char visible_text[256];
        int i;
        for (i = 0; i < draw_len && i < 255; i++) {
            visible_text[i] = w->text[w->scroll_offset + i];
        }
        visible_text[draw_len] = '\0';

        canvas_draw_string(canvas, x + padding, y + (wh - CANVAS_CHAR_H) / 2,
                           visible_text, COLOR_TEXT, COLOR_WHITE);
    }

    if (w->has_focus) {
        int cursor_x = x + padding + (w->cursor_pos - w->scroll_offset) * CANVAS_CHAR_W;
        int cursor_y = y + 3;
        int cursor_h = wh - 6;
        canvas_draw_vline(canvas, cursor_x, cursor_y, cursor_h, COLOR_BLACK);
    }
}

static void widget_draw_checkbox(widget_t *w, canvas_t *canvas, int ox, int oy)
{
    int x = w->rect.x + ox;
    int y = w->rect.y + oy;
    int box_size = 14;

    int box_y = y + (w->rect.h - box_size) / 2;
    if (box_y < y) box_y = y;

    color_t face = w->enabled ? COLOR_WHITE : COLOR_BUTTON_FACE;
    canvas_fill_rect(canvas, x, box_y, box_size, box_size, face);
    gfx_draw_sunken_rect(canvas, x, box_y, box_size, box_size,
                         COLOR_BUTTON_SHADOW, COLOR_BUTTON_HIGHLIGHT);

    if (w->checked) {
        int inner = 3;
        int x0 = x + inner;
        int y0 = box_y + inner;
        int x1 = x + box_size - inner - 1;
        int y1 = box_y + box_size - inner - 1;

        int i;
        for (i = -1; i <= 1; i++) {
            int px = x0, py = y0 + i;
            while (px <= x1 && py <= y1) {
                canvas_putpixel(canvas, px, py, COLOR_BLACK);
                px++; py++;
            }
        }
        for (i = -1; i <= 1; i++) {
            int px = x1 + i, py = y0;
            while (px >= x0 && py <= y1) {
                canvas_putpixel(canvas, px, py, COLOR_BLACK);
                px--; py++;
            }
        }
    }

    int label_x = x + box_size + 6;
    int label_y = y + (w->rect.h - CANVAS_CHAR_H) / 2;
    if (label_y < y) label_y = y;

    color_t label_color = w->enabled ? w->fg : COLOR_MID_GREY;
    canvas_draw_string(canvas, label_x, label_y, w->text, label_color, COLOR_WINDOW_BG);
}

static void widget_draw_groupbox(widget_t *w, canvas_t *canvas, int ox, int oy)
{
    int x = w->rect.x + ox;
    int y = w->rect.y + oy;
    int ww = w->rect.w;
    int wh = w->rect.h;

    gfx_draw_bevel_rect(canvas, x, y, ww, wh, COLOR_DARK_GREY, COLOR_WHITE);
    gfx_draw_bevel_rect(canvas, x + 1, y + 1, ww - 2, wh - 2, COLOR_WHITE, COLOR_DARK_GREY);

    if (w->text[0]) {
        int title_x = x + 10;
        int title_y = y - 7;
        if (title_y < 0) title_y = 0;

        canvas_fill_rect(canvas, title_x - 2, title_y, widget_strlen(w->text) * CANVAS_CHAR_W + 4,
                         CANVAS_CHAR_H, COLOR_WINDOW_BG);
        canvas_draw_string(canvas, title_x, title_y, w->text, COLOR_TEXT, COLOR_WINDOW_BG);
    }
}

void widget_mgr_draw_all(widget_mgr_t *mgr, canvas_t *canvas)
{
    if (!mgr || !canvas) return;

    widget_t *w = mgr->head;
    while (w) {
        if (w->visible) {
            switch (w->type) {
            case WIDGET_LABEL:
                widget_draw_label(w, canvas, WIDGET_OX, WIDGET_OY);
                break;
            case WIDGET_BUTTON:
                widget_draw_button(w, canvas, WIDGET_OX, WIDGET_OY);
                break;
            case WIDGET_TEXTBOX:
                widget_draw_textbox(w, canvas, WIDGET_OX, WIDGET_OY);
                break;
            case WIDGET_CHECKBOX:
                widget_draw_checkbox(w, canvas, WIDGET_OX, WIDGET_OY);
                break;
            case WIDGET_GROUPBOX:
                widget_draw_groupbox(w, canvas, WIDGET_OX, WIDGET_OY);
                break;
            default:
                break;
            }
        }
        w = w->next;
    }
}

static bool widget_hit(widget_t *w, int x, int y)
{
    if (!w || !w->visible || !w->enabled) return false;
    return (x >= w->rect.x && x < w->rect.x + w->rect.w &&
            y >= w->rect.y && y < w->rect.y + w->rect.h);
}

widget_t *widget_mgr_find_at(widget_mgr_t *mgr, int x, int y)
{
    if (!mgr) return NULL;

    widget_t *w = mgr->tail;
    while (w) {
        if (widget_hit(w, x, y)) {
            return w;
        }
        w = w->prev;
    }
    return NULL;
}

static void widget_mgr_clear_focus(widget_mgr_t *mgr)
{
    if (!mgr) return;
    if (mgr->focus_widget) {
        mgr->focus_widget->has_focus = false;
        mgr->focus_widget = NULL;
    }
}

bool widget_mgr_handle_click(widget_mgr_t *mgr, int x, int y)
{
    if (!mgr) return false;

    widget_t *hit = widget_mgr_find_at(mgr, x, y);
    bool changed = false;

    widget_mgr_clear_focus(mgr);

    if (hit && hit->enabled) {
        switch (hit->type) {
        case WIDGET_BUTTON:
            if (hit->on_click) {
                hit->pressed = true;
                hit->on_click(hit);
                hit->pressed = false;
            }
            changed = true;
            serial_write("[WIDGET] 按钮点击: ");
            serial_write(hit->text);
            serial_write("\n");
            break;

        case WIDGET_CHECKBOX:
            hit->checked = !hit->checked;
            if (hit->on_change) {
                hit->on_change(hit);
            }
            changed = true;
            serial_write("[WIDGET] 复选框切换\n");
            break;

        case WIDGET_TEXTBOX:
            hit->has_focus = true;
            mgr->focus_widget = hit;
            hit->cursor_pos = widget_strlen(hit->text);
            changed = true;
            serial_write("[WIDGET] 文本框获得焦点\n");
            break;

        default:
            break;
        }
    }

    return changed;
}

static bool widget_textbox_key(widget_t *w, char c)
{
    if (!w || !w->enabled) return false;

    int len = widget_strlen(w->text);

    if (c == '\b' || c == 127) {
        if (w->cursor_pos > 0 && len > 0) {
            int i;
            for (i = w->cursor_pos - 1; i < len; i++) {
                w->text[i] = w->text[i + 1];
            }
            w->cursor_pos--;
            return true;
        }
    } else if (c == '\r' || c == '\n') {
        if (w->on_change) {
            w->on_change(w);
        }
        return true;
    } else if (c >= 32 && c <= 126) {
        if (len < 254) {
            int i;
            for (i = len; i > w->cursor_pos; i--) {
                w->text[i] = w->text[i - 1];
            }
            w->text[w->cursor_pos] = c;
            w->text[len + 1] = '\0';
            w->cursor_pos++;
            return true;
        }
    }

    return false;
}

static bool widget_textbox_has_focus(widget_mgr_t *mgr)
{
    if (!mgr || !mgr->focus_widget) return false;
    return (mgr->focus_widget->type == WIDGET_TEXTBOX && mgr->focus_widget->has_focus);
}

bool widget_mgr_has_textbox_focus(widget_mgr_t *mgr)
{
    return widget_textbox_has_focus(mgr);
}

bool widget_mgr_handle_key(widget_mgr_t *mgr, char c)
{
    if (!mgr) return false;

    if (widget_textbox_has_focus(mgr)) {
        return widget_textbox_key(mgr->focus_widget, c);
    }

    return false;
}