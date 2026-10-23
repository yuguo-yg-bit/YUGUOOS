#pragma once

#include "types.h"
#include "canvas.h"

/* ═══════════════════════════════════════════════════════════════
   COM++ — YuguoOS 应用层开发框架
   对标 Win32 / Cocoa，封装所有内核能力为统一 API
   ═══════════════════════════════════════════════════════════════ */

typedef struct compp_window  compp_window_t;
typedef struct compp_widget  compp_widget_t;

typedef void (*compp_void_fn_t)(void);
typedef void (*compp_window_fn_t)(compp_window_t *win);
typedef void (*compp_widget_fn_t)(compp_widget_t *w);
typedef void (*compp_draw_fn_t)(compp_window_t *win, canvas_t *c);

/* ================================================================
   第1部分: 应用程序生命周期
   ================================================================ */

void compp_init(void);
void compp_run(void);
void compp_quit(void);

/* ================================================================
   第2部分: 窗口 API
   ================================================================ */

compp_window_t *compp_window_create(const char *title, int x, int y, int w, int h);
void            compp_window_destroy(compp_window_t *win);
void            compp_window_set_title(compp_window_t *win, const char *title);
void            compp_window_move(compp_window_t *win, int x, int y);
void            compp_window_resize(compp_window_t *win, int w, int h);
void            compp_window_show(compp_window_t *win);
void            compp_window_hide(compp_window_t *win);
void            compp_window_redraw(compp_window_t *win);
void            compp_window_focus(compp_window_t *win);

int  compp_window_get_width(compp_window_t *win);
int  compp_window_get_height(compp_window_t *win);
int  compp_window_get_content_width(compp_window_t *win);
int  compp_window_get_content_height(compp_window_t *win);

void compp_window_on_close(compp_window_t *win, compp_window_fn_t cb);
void compp_window_on_draw(compp_window_t *win, compp_draw_fn_t cb);
void compp_window_set_user_data(compp_window_t *win, void *data);
void *compp_window_get_user_data(compp_window_t *win);

/* ================================================================
   第3部分: 控件 API
   ================================================================ */

compp_widget_t *compp_button_create(compp_window_t *win, int x, int y,
                                     int w, int h, const char *text);
compp_widget_t *compp_label_create(compp_window_t *win, int x, int y,
                                    const char *text);
compp_widget_t *compp_checkbox_create(compp_window_t *win, int x, int y,
                                       const char *text, bool checked);
compp_widget_t *compp_textbox_create(compp_window_t *win, int x, int y,
                                      int w, const char *text);
compp_widget_t *compp_groupbox_create(compp_window_t *win, int x, int y,
                                       int w, int h, const char *title);

void        compp_widget_set_text(compp_widget_t *w, const char *text);
const char *compp_widget_get_text(compp_widget_t *w);
void        compp_widget_set_enabled(compp_widget_t *w, bool enabled);
bool        compp_widget_is_enabled(compp_widget_t *w);
bool        compp_widget_is_checked(compp_widget_t *w);
void        compp_widget_set_checked(compp_widget_t *w, bool checked);
void        compp_widget_set_color(compp_widget_t *w, color_t fg);

void compp_widget_on_click(compp_widget_t *w, compp_widget_fn_t cb);
void compp_widget_on_change(compp_widget_t *w, compp_widget_fn_t cb);

compp_window_t *compp_widget_get_owner(compp_widget_t *w);

/* ================================================================
   第4部分: 对话框 API
   ================================================================ */

void compp_message_box(const char *title, const char *message);

/* ================================================================
   第5部分: 绘制 API (在 on_draw 回调中使用)
   ================================================================ */

void compp_draw_text(canvas_t *c, int x, int y,
                     const char *text, color_t fg);
void compp_draw_rect(canvas_t *c, int x, int y,
                     int w, int h, color_t color);
void compp_draw_fill_rect(canvas_t *c, int x, int y,
                          int w, int h, color_t color);
void compp_draw_line(canvas_t *c, int x1, int y1,
                     int x2, int y2, color_t color);

/* ================================================================
   第6部分: 系统 API
   ================================================================ */

uint32_t compp_sys_memory_total_kb(void);
uint32_t compp_sys_memory_free_kb(void);
uint32_t compp_sys_memory_used_kb(void);
uint32_t compp_sys_screen_width(void);
uint32_t compp_sys_screen_height(void);
int      compp_sys_window_count(void);
const char *compp_sys_version(void);

/* ================================================================
   第7部分: 事件 & 定时器
   ================================================================ */

/* 注册一个空闲回调，在主循环每帧调用 */
void compp_on_idle(compp_void_fn_t cb);