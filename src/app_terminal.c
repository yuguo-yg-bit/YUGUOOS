#include "app.h"
#include "compp.h"
#include "serial.h"

static char term_output[512];
static int  term_out_pos = 0;

static void term_append(const char *s)
{
    int i = 0;
    while (s[i] && term_out_pos < 510) {
        term_output[term_out_pos++] = s[i++];
    }
    term_output[term_out_pos] = '\0';
}

static void on_cmd_enter(compp_widget_t *w)
{
    const char *cmd = compp_widget_get_text(w);
    if (!cmd || cmd[0] == '\0') return;

    serial_write("[APP] Terminal cmd: ");
    serial_write(cmd);
    serial_write("\n");

    term_append("> ");
    term_append(cmd);
    term_append("\n");

    compp_widget_set_text(w, "");
}

static void on_clear(compp_widget_t *w)
{
    (void)w;
    term_out_pos = 0;
    term_output[0] = '\0';
    serial_write("[APP] Terminal: clear\n");
}

static void term_draw_output(compp_window_t *win, canvas_t *c)
{
    (void)win;
    if (term_output[0] == '\0') {
        compp_draw_text(c, 4, 4, "Type a command and press Enter.", COLOR_MID_GREY);
        return;
    }
    int y = 4;
    int start = 0;
    for (int i = 0; term_output[i]; i++) {
        if (term_output[i] == '\n' || i == 510) {
            int len = i - start;
            if (len > 60) len = 60;
            char line[64];
            int li;
            for (li = 0; li < len && li < 63; li++) {
                line[li] = term_output[start + li];
            }
            line[li] = '\0';
            compp_draw_text(c, 4, y, line, COLOR_TEXT);
            y += 16;
            start = i + 1;
            if (term_output[i] == '\0') break;
        }
    }
}

void app_terminal_run(void)
{
    int sw = (int)compp_sys_screen_width();
    int sh = (int)compp_sys_screen_height();
    int cx = sw / 2;
    int cy = sh / 2;

    compp_window_t *win = compp_window_create(
        "Terminal", cx + 80, cy + 120, 340, 280);
    if (!win) return;

    compp_window_on_draw(win, term_draw_output);

    compp_label_create(win, 10, 4, "YuguoOS Terminal v1.0");
    compp_label_create(win, 10, 20, "COM++ Shell");

    compp_widget_set_color(
        compp_label_create(win, 10, 42, "------------------------------"),
        COLOR_DARK_GREY);

    compp_label_create(win, 10, 64, "> ");

    compp_widget_t *input = compp_textbox_create(win, 28, 62, 280, "");
    compp_widget_on_change(input, on_cmd_enter);

    compp_widget_t *clear_btn = compp_button_create(win, 10, 215, 80, 24, "Clear");
    compp_widget_on_click(clear_btn, on_clear);

    serial_write("[APP] Terminal started\n");
}