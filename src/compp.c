#include "compp.h"
#include "wm.h"
#include "widget.h"
#include "win.h"
#include "gfx.h"
#include "fb.h"
#include "serial.h"
#include "vmm.h"
#include "pmm.h"
#include "input.h"
#include "kb.h"
#include "mouse.h"

/* ═══════════════════════════════════════════════════════════════
   COM++ 内部结构定义
   ═══════════════════════════════════════════════════════════════ */

struct compp_window {
    win_t          *win;
    widget_mgr_t   *widgets;
    compp_draw_fn_t on_draw;
    compp_window_fn_t on_close;
    void           *user_data;
};

struct compp_widget {
    widget_t        *widget;
    compp_window_t  *owner;
    compp_widget_fn_t on_click;
    compp_widget_fn_t on_change;
};

static bool           compp_running = false;
static compp_void_fn_t compp_idle_cb = NULL;

/* ═══════════════════════════════════════════════════════════════
   内部辅助: 字符串长度 (freestanding, 无 libc)
   ═══════════════════════════════════════════════════════════════ */

static int compp_strlen(const char *s)
{
    int len = 0;
    if (!s) return 0;
    while (s[len]) len++;
    return len;
}

/* ═══════════════════════════════════════════════════════════════
   内部: COM++ 窗口绘制回调 (连接到 win_t)
   ═══════════════════════════════════════════════════════════════ */

static void compp_draw_content(win_t *win)
{
    compp_window_t *cw = (compp_window_t *)win->user_data;
    if (!cw) return;

    if (cw->widgets) {
        widget_mgr_draw_all(cw->widgets, win->canvas);
    }

    if (cw->on_draw) {
        cw->on_draw(cw, win->canvas);
    }
}

/* ═══════════════════════════════════════════════════════════════
   内部: COM++ 窗口清理回调 (连接到 win_t)
   ═══════════════════════════════════════════════════════════════ */

static void compp_cleanup(win_t *win)
{
    compp_window_t *cw = (compp_window_t *)win->user_data;
    if (!cw) return;

    if (cw->on_close) {
        cw->on_close(cw);
    }

    kfree(cw);
    win->user_data = NULL;
}

/* ═══════════════════════════════════════════════════════════════
   内部: Widget 回调桥接 (widget_t → compp_widget_t)
   ═══════════════════════════════════════════════════════════════ */

static void compp_widget_click_bridge(widget_t *w)
{
    compp_widget_t *cw = (compp_widget_t *)w->user_data;
    if (cw && cw->on_click) {
        cw->on_click(cw);
    }
}

static void compp_widget_change_bridge(widget_t *w)
{
    compp_widget_t *cw = (compp_widget_t *)w->user_data;
    if (cw && cw->on_change) {
        cw->on_change(cw);
    }
}

/* ═══════════════════════════════════════════════════════════════
   第1部分: 应用程序生命周期
   ═══════════════════════════════════════════════════════════════ */

void compp_init(void)
{
    serial_write("[COM++] 初始化应用框架...\n");

    input_init();
    wm_init();

    if (!fb_available()) {
        serial_write("[COM++] 帧缓冲不可用!\n");
        return;
    }

    mouse_init();
    compp_running = true;

    serial_write("[COM++] COM++ 框架就绪 v1.0\n");
}

void compp_run(void)
{
    serial_write("[COM++] 进入主事件循环\n");

    bool prev_left = false;
    bool dragging = false;
    int  drag_off_x = 0, drag_off_y = 0;
    win_t *drag_win = NULL;

    while (compp_running) {
        bool need_redraw = false;

        if (kb_haschar()) {
            char c = kb_getchar();
            input_push_key(c);

            input_event_t ev;
            while (input_poll(&ev)) {
                wm_handle_input(&ev);
                need_redraw = true;
            }
        }

        mouse_poll();
        mouse_state_t ms;
        mouse_get_state(&ms);

        if (ms.valid) {
            bool left_now = ms.left;

            if (left_now && !prev_left) {
                wm_handle_mouse(ms.x, ms.y, true, false);

                win_t *fw = wm_get_focused();
                if (fw && win_hit_titlebar(fw, ms.x, ms.y)) {
                    dragging = true;
                    drag_win = fw;
                    drag_off_x = ms.x - drag_win->rect.x;
                    drag_off_y = ms.y - drag_win->rect.y;
                }
                need_redraw = true;
            }

            if (!left_now && prev_left) {
                wm_handle_mouse(ms.x, ms.y, false, true);
                dragging = false;
                drag_win = NULL;
            }

            if (dragging && drag_win) {
                int new_x = ms.x - drag_off_x;
                int new_y = ms.y - drag_off_y;
                if (new_x != drag_win->rect.x || new_y != drag_win->rect.y) {
                    win_move(drag_win, new_x, new_y);
                    need_redraw = true;
                }
            }

            prev_left = left_now;
            if (ms.valid) need_redraw = true;
        }

        if (compp_idle_cb) {
            compp_idle_cb();
        }

        if (need_redraw) {
            wm_redraw_all();
            wm_present();
            mouse_draw_at();
        } else {
            mouse_update_and_draw();
        }

        for (volatile int i = 0; i < 1000; i++) {
            asm volatile("pause");
        }
    }

    serial_write("[COM++] 主事件循环退出\n");
}

void compp_quit(void)
{
    compp_running = false;
    serial_write("[COM++] 请求退出\n");
}

/* ═══════════════════════════════════════════════════════════════
   第2部分: 窗口 API
   ═══════════════════════════════════════════════════════════════ */

compp_window_t *compp_window_create(const char *title, int x, int y, int w, int h)
{
    compp_window_t *cw = (compp_window_t *)kmalloc(sizeof(compp_window_t));
    if (!cw) return NULL;

    cw->widgets = widget_mgr_create();
    if (!cw->widgets) {
        kfree(cw);
        return NULL;
    }

    cw->win = wm_create_window(title, x, y, w, h);
    if (!cw->win) {
        widget_mgr_destroy(cw->widgets);
        kfree(cw);
        return NULL;
    }

    cw->on_draw  = NULL;
    cw->on_close = NULL;
    cw->user_data = NULL;

    cw->win->draw_content = compp_draw_content;
    cw->win->cleanup      = compp_cleanup;
    cw->win->widgets      = cw->widgets;
    cw->win->user_data    = cw;

    serial_write("[COM++] 创建窗口: ");
    serial_write(title ? title : "");
    serial_write("\n");
    return cw;
}

void compp_window_destroy(compp_window_t *win)
{
    if (!win || !win->win) return;
    wm_destroy_window(win->win);
}

void compp_window_set_title(compp_window_t *win, const char *title)
{
    if (!win || !win->win) return;
    win_set_title(win->win, title);
}

void compp_window_move(compp_window_t *win, int x, int y)
{
    if (!win || !win->win) return;
    win_move(win->win, x, y);
}

void compp_window_resize(compp_window_t *win, int w, int h)
{
    if (!win || !win->win) return;
    win_resize(win->win, w, h);
}

void compp_window_show(compp_window_t *win)
{
    if (!win || !win->win) return;
    win->win->visible = true;
    win->win->dirty = true;
    wm_redraw_all();
    wm_present();
}

void compp_window_hide(compp_window_t *win)
{
    if (!win || !win->win) return;
    win->win->visible = false;
    wm_redraw_all();
    wm_present();
}

void compp_window_redraw(compp_window_t *win)
{
    if (!win || !win->win) return;
    win->win->dirty = true;
    wm_redraw_all();
    wm_present();
}

void compp_window_focus(compp_window_t *win)
{
    if (!win || !win->win) return;
    wm_focus_window(win->win);
    wm_redraw_all();
    wm_present();
}

int compp_window_get_width(compp_window_t *win)
{
    if (!win || !win->win) return 0;
    return win->win->rect.w;
}

int compp_window_get_height(compp_window_t *win)
{
    if (!win || !win->win) return 0;
    return win->win->rect.h;
}

int compp_window_get_content_width(compp_window_t *win)
{
    if (!win || !win->win) return 0;
    return win_content_w(win->win);
}

int compp_window_get_content_height(compp_window_t *win)
{
    if (!win || !win->win) return 0;
    return win_content_h(win->win);
}

void compp_window_on_close(compp_window_t *win, compp_window_fn_t cb)
{
    if (!win) return;
    win->on_close = cb;
}

void compp_window_on_draw(compp_window_t *win, compp_draw_fn_t cb)
{
    if (!win) return;
    win->on_draw = cb;
}

void compp_window_set_user_data(compp_window_t *win, void *data)
{
    if (!win) return;
    win->user_data = data;
}

void *compp_window_get_user_data(compp_window_t *win)
{
    if (!win) return NULL;
    return win->user_data;
}

/* ═══════════════════════════════════════════════════════════════
   第3部分: 控件 API
   ═══════════════════════════════════════════════════════════════ */

static compp_widget_t *compp_widget_alloc(compp_window_t *win)
{
    compp_widget_t *cw = (compp_widget_t *)kmalloc(sizeof(compp_widget_t));
    if (!cw) return NULL;

    cw->widget    = NULL;
    cw->owner     = win;
    cw->on_click  = NULL;
    cw->on_change = NULL;
    return cw;
}

compp_widget_t *compp_button_create(compp_window_t *win, int x, int y,
                                     int w, int h, const char *text)
{
    if (!win || !win->widgets) return NULL;

    compp_widget_t *cw = compp_widget_alloc(win);
    if (!cw) return NULL;

    cw->widget = widget_create_button(win->widgets, x, y, w, h,
                                       text, compp_widget_click_bridge);
    if (!cw->widget) {
        kfree(cw);
        return NULL;
    }

    cw->widget->user_data = cw;
    return cw;
}

compp_widget_t *compp_label_create(compp_window_t *win, int x, int y,
                                    const char *text)
{
    if (!win || !win->widgets) return NULL;

    compp_widget_t *cw = compp_widget_alloc(win);
    if (!cw) return NULL;

    cw->widget = widget_create_label(win->widgets, x, y, text);
    if (!cw->widget) {
        kfree(cw);
        return NULL;
    }

    cw->widget->user_data = cw;
    return cw;
}

compp_widget_t *compp_checkbox_create(compp_window_t *win, int x, int y,
                                       const char *text, bool checked)
{
    if (!win || !win->widgets) return NULL;

    compp_widget_t *cw = compp_widget_alloc(win);
    if (!cw) return NULL;

    cw->widget = widget_create_checkbox(win->widgets, x, y,
                                         text, checked,
                                         compp_widget_change_bridge);
    if (!cw->widget) {
        kfree(cw);
        return NULL;
    }

    cw->widget->user_data = cw;
    return cw;
}

compp_widget_t *compp_textbox_create(compp_window_t *win, int x, int y,
                                      int w, const char *text)
{
    if (!win || !win->widgets) return NULL;

    compp_widget_t *cw = compp_widget_alloc(win);
    if (!cw) return NULL;

    cw->widget = widget_create_textbox(win->widgets, x, y, w,
                                        text, compp_widget_change_bridge);
    if (!cw->widget) {
        kfree(cw);
        return NULL;
    }

    cw->widget->user_data = cw;
    return cw;
}

compp_widget_t *compp_groupbox_create(compp_window_t *win, int x, int y,
                                       int w, int h, const char *title)
{
    if (!win || !win->widgets) return NULL;

    compp_widget_t *cw = compp_widget_alloc(win);
    if (!cw) return NULL;

    cw->widget = widget_create_groupbox(win->widgets, x, y, w, h, title);
    if (!cw->widget) {
        kfree(cw);
        return NULL;
    }

    cw->widget->user_data = cw;
    return cw;
}

void compp_widget_set_text(compp_widget_t *w, const char *text)
{
    if (!w || !w->widget) return;
    widget_set_text(w->widget, text);
}

const char *compp_widget_get_text(compp_widget_t *w)
{
    if (!w || !w->widget) return "";
    return widget_get_text(w->widget);
}

void compp_widget_set_enabled(compp_widget_t *w, bool enabled)
{
    if (!w || !w->widget) return;
    widget_set_enabled(w->widget, enabled);
}

bool compp_widget_is_enabled(compp_widget_t *w)
{
    if (!w || !w->widget) return false;
    return w->widget->enabled;
}

bool compp_widget_is_checked(compp_widget_t *w)
{
    if (!w || !w->widget) return false;
    return w->widget->checked;
}

void compp_widget_set_checked(compp_widget_t *w, bool checked)
{
    if (!w || !w->widget) return;
    w->widget->checked = checked;
}

void compp_widget_set_color(compp_widget_t *w, color_t fg)
{
    if (!w || !w->widget) return;
    widget_set_fg(w->widget, fg);
}

void compp_widget_on_click(compp_widget_t *w, compp_widget_fn_t cb)
{
    if (!w) return;
    w->on_click = cb;
}

void compp_widget_on_change(compp_widget_t *w, compp_widget_fn_t cb)
{
    if (!w) return;
    w->on_change = cb;
}

compp_window_t *compp_widget_get_owner(compp_widget_t *w)
{
    if (!w) return NULL;
    return w->owner;
}

/* ═══════════════════════════════════════════════════════════════
   第4部分: 对话框 API
   ═══════════════════════════════════════════════════════════════ */

void compp_message_box(const char *title, const char *message)
{
    serial_write("[COM++] 消息框: ");
    serial_write(title ? title : "");
    serial_write(" - ");
    serial_write(message ? message : "");
    serial_write("\n");

    int msg_w = (compp_strlen(message) + 2) * CANVAS_CHAR_W + 40;
    if (msg_w < 200) msg_w = 200;

    compp_window_t *dlg = compp_window_create(title,
        ((int)fb_get_width() - msg_w) / 2,
        ((int)fb_get_height() - 140) / 2,
        msg_w, 140);

    if (!dlg) return;

    compp_label_create(dlg, 20, 20, message);
    compp_widget_t *btn = compp_button_create(dlg,
        msg_w / 2 - 40, 90, 80, 24, "确定");
    (void)btn;

    compp_window_show(dlg);
    compp_window_focus(dlg);
}

/* ═══════════════════════════════════════════════════════════════
   第5部分: 绘制 API
   ═══════════════════════════════════════════════════════════════ */

void compp_draw_text(canvas_t *c, int x, int y,
                     const char *text, color_t fg)
{
    canvas_draw_string(c, x, y, text, fg, COLOR_WINDOW_BG);
}

void compp_draw_rect(canvas_t *c, int x, int y,
                     int w, int h, color_t color)
{
    canvas_draw_rect(c, x, y, w, h, color);
}

void compp_draw_fill_rect(canvas_t *c, int x, int y,
                          int w, int h, color_t color)
{
    canvas_fill_rect(c, x, y, w, h, color);
}

void compp_draw_line(canvas_t *c, int x1, int y1,
                     int x2, int y2, color_t color)
{
    int dx = x2 - x1;
    int dy = y2 - y1;
    int steps = (dx < 0 ? -dx : dx) > (dy < 0 ? -dy : dy)
                ? (dx < 0 ? -dx : dx) : (dy < 0 ? -dy : dy);
    if (steps == 0) {
        canvas_putpixel(c, x1, y1, color);
        return;
    }

    for (int i = 0; i <= steps; i++) {
        int px = x1 + (dx * i) / steps;
        int py = y1 + (dy * i) / steps;
        canvas_putpixel(c, px, py, color);
    }
}

/* ═══════════════════════════════════════════════════════════════
   第6部分: 系统 API
   ═══════════════════════════════════════════════════════════════ */

uint32_t compp_sys_memory_total_kb(void)
{
    return (uint32_t)(pmm_total_pages() * 4);
}

uint32_t compp_sys_memory_free_kb(void)
{
    return (uint32_t)(pmm_get_free_pages() * 4);
}

uint32_t compp_sys_memory_used_kb(void)
{
    return compp_sys_memory_total_kb() - compp_sys_memory_free_kb();
}

uint32_t compp_sys_screen_width(void)
{
    return fb_get_width();
}

uint32_t compp_sys_screen_height(void)
{
    return fb_get_height();
}

int compp_sys_window_count(void)
{
    return wm_window_count();
}

const char *compp_sys_version(void)
{
    return "YuguoOS v0.3.0 / COM++ v1.0";
}

/* ═══════════════════════════════════════════════════════════════
   第7部分: 事件 & 定时器
   ═══════════════════════════════════════════════════════════════ */

void compp_on_idle(compp_void_fn_t cb)
{
    compp_idle_cb = cb;
}