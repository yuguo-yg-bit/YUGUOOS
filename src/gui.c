#include "gui.h"
#include "wm.h"
#include "input.h"
#include "kb.h"
#include "mouse.h"
#include "serial.h"
#include "fb.h"
#include "gfx.h"
#include "pmm.h"

static void uint_to_str(uint32_t val, char *buf)
{
    if (val == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }
    char tmp[12];
    int pos = 0;
    while (val > 0) {
        tmp[pos++] = (char)('0' + (val % 10));
        val /= 10;
    }
    int i = 0;
    while (pos > 0) {
        buf[i++] = tmp[--pos];
    }
    buf[i] = '\0';
}

static void draw_sysinfo_content(win_t *win)
{
    char buf[64];
    int y = 4;
    int i;

    win_content_draw_text(win, 4, y, "YuguoOS v0.3.0", COLOR_TEXT);
    y += 18;

    win_content_draw_text(win, 4, y, "-------------------", COLOR_DARK_GREY);
    y += 16;

    win_content_draw_text(win, 4, y, "物理内存:", COLOR_TEXT);

    for (i = 0; i < 63; i++) buf[i] = '\0';
    char *bp = buf;
    uint_to_str((uint32_t)(pmm_total_pages() * 4), bp);
    while (*bp) bp++;
    *bp++ = 'K'; *bp++ = 'B'; *bp++ = ' '; *bp++ = '/'; *bp++ = ' ';
    uint_to_str((uint32_t)((pmm_total_pages() - pmm_get_free_pages()) * 4), bp);
    while (*bp) bp++;
    *bp++ = 'K'; *bp++ = 'B'; *bp++ = ' '; *bp++ = '(';
    uint_to_str((uint32_t)(pmm_get_free_pages() * 4), bp);
    while (*bp) bp++;
    *bp++ = 'K'; *bp++ = 'B'; *bp++ = ')'; *bp = '\0';
    win_content_draw_text(win, 4, y + 18, buf, COLOR_HIGHLIGHT);
    y += 38;

    win_content_draw_text(win, 4, y, "屏幕: ", COLOR_TEXT);
    for (i = 0; i < 63; i++) buf[i] = '\0';
    bp = buf;
    uint_to_str(fb_get_width(), bp); while (*bp) bp++;
    *bp++ = 'x';
    uint_to_str(fb_get_height(), bp); while (*bp) bp++;
    *bp++ = 'x';
    uint_to_str(fb_get_bpp_bytes() * 8, bp);
    *bp = '\0';
    win_content_draw_text(win, 4 + 80, y, buf, COLOR_HIGHLIGHT);
    y += 18;

    win_content_draw_text(win, 4, y, "窗口数: ", COLOR_TEXT);
    for (i = 0; i < 63; i++) buf[i] = '\0';
    uint_to_str((uint32_t)wm_window_count(), buf);
    win_content_draw_text(win, 4 + 80, y, buf, COLOR_HIGHLIGHT);
    y += 18;

    win_content_draw_text(win, 4, y, "YuguoOS Team (c) 2026", COLOR_MID_GREY);
}

static void draw_filemgr_content(win_t *win)
{
    int y = 4;
    win_content_draw_text(win, 4, y, "/", COLOR_TEXT); y += 17;
    win_content_draw_text(win, 4, y, "  dev/", COLOR_DARK_GREY); y += 17;
    win_content_draw_text(win, 4, y, "    keyboard", COLOR_MID_GREY); y += 17;
    win_content_draw_text(win, 4, y, "    serial", COLOR_MID_GREY); y += 17;
    win_content_draw_text(win, 4, y, "    framebuffer", COLOR_MID_GREY); y += 17;
    win_content_draw_text(win, 4, y, "  tmp/", COLOR_DARK_GREY); y += 17;
    win_content_draw_text(win, 4, y, "  mnt/", COLOR_DARK_GREY); y += 17;
}

static void draw_terminal_content(win_t *win)
{
    int y = 4;
    win_content_draw_text(win, 4, y, "YuguoOS Terminal v0.1", COLOR_TEXT);
    y += 20;
    win_content_draw_text(win, 4, y, "> ", COLOR_HIGHLIGHT);

    if (win->content_pos > 0) {
        win_content_draw_text(win, 4 + 16, y, win->content_buf, COLOR_TEXT);
    }
}

static void draw_settings_content(win_t *win)
{
    int y = 4;
    win_content_draw_text(win, 4, y, "YuguoOS 设置面板", COLOR_TEXT);
    y += 20;
    win_content_draw_text(win, 4, y, "--------------------------------", COLOR_MID_GREY);
    y += 18;
    win_content_draw_text(win, 4, y, " 桌面背景:   蓝色", COLOR_TEXT);
    y += 18;
    win_content_draw_text(win, 4, y, " 配色方案:   Windows 3.0 经典", COLOR_TEXT);
    y += 18;
    win_content_draw_text(win, 4, y, " 窗口特效:   无", COLOR_TEXT);
    y += 18;
    win_content_draw_text(win, 4, y, " 鼠标速度:   中", COLOR_TEXT);
    y += 18;
    win_content_draw_text(win, 4, y, " 虚拟内存:   关闭", COLOR_TEXT);
    y += 18;
    win_content_draw_text(win, 4, y, "  更多设置开发中...", COLOR_MID_GREY);
}

static void term_input_callback(win_t *win, char c)
{
    (void)c;
    win->content_pos = 0;
    win->content_buf[0] = '\0';
}

void gui_start(void)
{
    serial_write("[GUI] 初始化桌面环境...\n");

    input_init();
    wm_init();

    if (!fb_available()) {
        serial_write("[GUI] 帧缓冲不可用，无法启动桌面\n");
        return;
    }

    mouse_init();

    uint32_t sw = fb_get_width();
    uint32_t sh = fb_get_height();
    int cx = (int)sw / 2;
    int cy = (int)sh / 2;

    win_t *w1 = wm_create_window("系统信息", cx - 290, cy - 240, 580, 350);
    win_t *w2 = wm_create_window("文件管理器", cx - 280, cy + 120, 340, 260);
    win_t *w3 = wm_create_window("终端", cx + 70, cy + 120, 340, 260);
    win_t *w4 = wm_create_window("YuguoOS 设置", cx - 220, cy - 100, 440, 320);

    if (w1) { w1->draw_content = draw_sysinfo_content; }
    if (w2) { w2->draw_content = draw_filemgr_content; }
    if (w3) { w3->draw_content = draw_terminal_content; w3->input_char = term_input_callback; }
    if (w4) { w4->draw_content = draw_settings_content; }

    serial_write("[GUI] 桌面环境就绪，共 ");
    char buf[8];
    int cnt = wm_window_count();
    int pos = 0;
    if (cnt >= 10) { buf[pos++] = (char)('0' + cnt / 10); }
    buf[pos++] = (char)('0' + cnt % 10);
    buf[pos] = '\0';
    serial_write(buf);
    serial_write(" 个窗口\n");

    wm_composite();
    wm_present();
}

void gui_run(void)
{
    serial_write("[GUI] 进入桌面主循环\n");

    bool prev_left = false;
    bool dragging = false;
    int drag_off_x = 0, drag_off_y = 0;
    win_t *drag_win = NULL;

    for (;;) {
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

            if (ms.valid) {
                need_redraw = true;
            }
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
}