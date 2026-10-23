#include "app.h"
#include "compp.h"
#include "serial.h"

static int recycle_item_count = 0;

static void on_empty(compp_widget_t *w)
{
    recycle_item_count = 0;
    compp_message_box("Recycle Bin", "Recycle bin emptied.");
    serial_write("[APP] RecycleBin: emptied\n");
    (void)w;
}

static void on_refresh_bin(compp_widget_t *w)
{
    (void)w;
    serial_write("[APP] RecycleBin: refresh (");
    serial_write(recycle_item_count == 0 ? "empty" : "items present");
    serial_write(")\n");
}

static void on_close_rc(compp_widget_t *w)
{
    compp_window_t *win = compp_widget_get_owner(w);
    compp_window_destroy(win);
    serial_write("[APP] RecycleBin: close\n");
}

void app_recycle_run(void)
{
    int sw = (int)compp_sys_screen_width();
    int sh = (int)compp_sys_screen_height();
    int cx = sw / 2;
    int cy = sh / 2;

    compp_window_t *win = compp_window_create(
        "Recycle Bin", cx + 280, cy + 130, 340, 260);
    if (!win) return;

    compp_groupbox_create(win, 10, 8, 320, 170, "Contents");

    if (recycle_item_count == 0) {
        compp_widget_set_color(
            compp_label_create(win, 80, 60, "Recycle Bin is empty"),
            COLOR_MID_GREY);
    }

    char count_buf[32];
    count_buf[0] = 'I'; count_buf[1] = 't'; count_buf[2] = 'e';
    count_buf[3] = 'm'; count_buf[4] = 's'; count_buf[5] = ':'; count_buf[6] = ' ';
    count_buf[7] = (char)('0' + recycle_item_count);
    count_buf[8] = '\0';
    compp_label_create(win, 100, 100, count_buf);

    compp_widget_t *empty_btn = compp_button_create(win, 10, 195, 100, 24, "Empty Bin");
    compp_widget_on_click(empty_btn, on_empty);

    compp_widget_t *refresh_btn = compp_button_create(win, 120, 195, 80, 24, "Refresh");
    compp_widget_on_click(refresh_btn, on_refresh_bin);

    compp_widget_t *close_btn = compp_button_create(win, 210, 195, 80, 24, "Close");
    compp_widget_on_click(close_btn, on_close_rc);

    serial_write("[APP] Recycle Bin started\n");
}