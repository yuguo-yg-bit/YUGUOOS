#include "app.h"
#include "compp.h"
#include "pmm.h"
#include "serial.h"

static void uint_to_str_kb(uint32_t val, char *buf)
{
    if (val == 0) { buf[0] = '0'; buf[1] = '\0'; return; }
    char tmp[12];
    int pos = 0;
    while (val > 0) { tmp[pos++] = (char)('0' + (val % 10)); val /= 10; }
    int i = 0;
    while (pos > 0) { buf[i++] = tmp[--pos]; }
    buf[i] = '\0';
}

static void build_mem_str(char *buf, int max)
{
    int bp = 0;
    int remain = max;
    uint_to_str_kb(compp_sys_memory_total_kb(), buf + bp);
    while (buf[bp] && remain > 1) { bp++; remain--; }
    if (remain < 3) return;
    buf[bp++] = 'K'; buf[bp++] = 'B'; buf[bp++] = ' '; buf[bp++] = '/'; buf[bp++] = ' '; remain -= 5;
    uint_to_str_kb(compp_sys_memory_used_kb(), buf + bp);
    while (buf[bp] && remain > 1) { bp++; remain--; }
    if (remain < 3) return;
    buf[bp++] = 'K'; buf[bp++] = 'B'; buf[bp++] = ' '; buf[bp++] = '('; remain -= 4;
    uint_to_str_kb(compp_sys_memory_free_kb(), buf + bp);
    while (buf[bp] && remain > 1) { bp++; remain--; }
    if (remain < 4) return;
    buf[bp++] = 'K'; buf[bp++] = 'B'; buf[bp++] = ')'; buf[bp] = '\0';
}

static void refresh_info(compp_window_t *win,
                          compp_widget_t *mem_lbl,
                          compp_widget_t *win_lbl)
{
    (void)win;
    char buf[80];
    build_mem_str(buf, 80);
    compp_widget_set_text(mem_lbl, buf);

    int cnt = compp_sys_window_count();
    buf[0] = (char)('0' + cnt / 10);
    buf[1] = (char)('0' + cnt % 10);
    buf[2] = ' '; buf[3] = 'w'; buf[4] = 'i'; buf[5] = 'n'; buf[6] = 'd';
    buf[7] = 'o'; buf[8] = 'w'; buf[9] = 's'; buf[10] = '\0';
    compp_widget_set_text(win_lbl, buf);
}

static void on_refresh(compp_widget_t *w)
{
    compp_window_t *win = compp_widget_get_owner(w);
    refresh_info(win,
        (compp_widget_t *)compp_window_get_user_data(win), NULL);
    serial_write("[APP] Desktop: refresh\n");
}

static void on_close_client(compp_widget_t *w)
{
    compp_window_t *win = compp_widget_get_owner(w);
    compp_window_destroy(win);
    serial_write("[APP] Desktop: close\n");
}

void app_desktop_run(void)
{
    int sw = (int)compp_sys_screen_width();
    int sh = (int)compp_sys_screen_height();
    int cx = sw / 2;
    int cy = sh / 2;

    compp_window_t *win = compp_window_create(
        "This PC", cx - 270, cy - 230, 540, 380);
    if (!win) return;

    compp_label_create(win, 12, 4,  "YuguoOS v0.3.0");
    compp_widget_set_color(
        compp_label_create(win, 12, 22, "------------------------------"),
        COLOR_DARK_GREY);

    compp_label_create(win, 12, 44, "Memory:");
    compp_widget_t *mem = compp_label_create(win, 100, 44, "???");
    compp_widget_set_color(mem, COLOR_HIGHLIGHT);

    char scr_buf[24];
    int bp = 0;
    char tmp[12];
    uint32_t fw = compp_sys_screen_width();
    uint32_t fh = compp_sys_screen_height();
    uint_to_str_kb(fw, tmp);
    int ti = 0; while (tmp[ti]) scr_buf[bp++] = tmp[ti++];
    scr_buf[bp++] = 'x';
    uint_to_str_kb(fh, tmp);
    ti = 0; while (tmp[ti]) scr_buf[bp++] = tmp[ti++];
    scr_buf[bp++] = 'x'; scr_buf[bp++] = '3'; scr_buf[bp++] = '2';
    scr_buf[bp] = '\0';

    compp_label_create(win, 12, 64, "Screen:");
    compp_widget_t *scr = compp_label_create(win, 100, 64, scr_buf);
    compp_widget_set_color(scr, COLOR_HIGHLIGHT);

    compp_label_create(win, 12, 84, "Windows:");
    compp_widget_t *win_cnt = compp_label_create(win, 100, 84, "???");
    compp_widget_set_color(win_cnt, COLOR_HIGHLIGHT);

    compp_label_create(win, 12, 106, "COM++ Framework v1.0");
    compp_widget_set_color(
        compp_label_create(win, 12, 124, "YuguoOS Team (c) 2026"),
        COLOR_MID_GREY);

    compp_groupbox_create(win, 10, 148, 520, 130, "Drives");

    compp_label_create(win, 22, 176, "[C:] System Disk");
    compp_widget_set_color(
        compp_label_create(win, 22, 196, "     Type: Local Disk"), COLOR_MID_GREY);
    compp_widget_set_color(
        compp_label_create(win, 22, 216, "     FS: VFS (Virtual)"), COLOR_MID_GREY);

    compp_label_create(win, 240, 176, "[D:] Data Disk");
    compp_widget_set_color(
        compp_label_create(win, 240, 196, "     Type: Virtual Disk"), COLOR_MID_GREY);
    compp_widget_set_color(
        compp_label_create(win, 240, 216, "     Size: 256 MB"), COLOR_MID_GREY);

    compp_widget_t *refresh = compp_button_create(win, 320, 296, 80, 24, "Refresh");
    compp_widget_on_click(refresh, on_refresh);

    compp_widget_t *close_btn = compp_button_create(win, 410, 296, 80, 24, "Close");
    compp_widget_on_click(close_btn, on_close_client);

    compp_window_set_user_data(win, mem);
    refresh_info(win, mem, win_cnt);

    serial_write("[APP] This PC started\n");
}