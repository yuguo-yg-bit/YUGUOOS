#include "app.h"
#include "compp.h"
#include "serial.h"

static bool fx_enabled    = false;
static bool vmem_enabled  = false;
static char hostname[256] = "YuguoOS";

static void on_apply(compp_widget_t *w)
{
    (void)w;
    serial_write("[APP] Settings: Apply - FX=");
    serial_write(fx_enabled ? "ON" : "OFF");
    serial_write(" VMEM=");
    serial_write(vmem_enabled ? "ON" : "OFF");
    serial_write(" Host=");
    serial_write(hostname);
    serial_write("\n");
    compp_message_box("Settings", "Settings applied!");
}

static void on_cancel(compp_widget_t *w)
{
    (void)w;
    serial_write("[APP] Settings: Cancel\n");
}

static void on_about(compp_widget_t *w)
{
    (void)w;
    compp_message_box("About YuguoOS",
        "YuguoOS v0.3.0\nCOM++ Framework v1.0\n(c) 2026 YuguoOS Team");
    serial_write("[APP] Settings: About\n");
}

static void on_fx_toggle(compp_widget_t *w)
{
    fx_enabled = compp_widget_is_checked(w);
    serial_write("[APP] Settings: WindowFX=");
    serial_write(fx_enabled ? "ON" : "OFF");
    serial_write("\n");
}

static void on_vmem_toggle(compp_widget_t *w)
{
    vmem_enabled = compp_widget_is_checked(w);
    serial_write("[APP] Settings: VMEM=");
    serial_write(vmem_enabled ? "ON" : "OFF");
    serial_write("\n");
}

static void on_hostname_change(compp_widget_t *w)
{
    const char *txt = compp_widget_get_text(w);
    int i = 0;
    while (i < 254 && txt[i]) { hostname[i] = txt[i]; i++; }
    hostname[i] = '\0';
    serial_write("[APP] Settings: hostname=");
    serial_write(hostname);
    serial_write("\n");
}

void app_settings_run(void)
{
    int sw = (int)compp_sys_screen_width();
    int sh = (int)compp_sys_screen_height();
    int cx = sw / 2;
    int cy = sh / 2;

    compp_window_t *win = compp_window_create(
        "YuguoOS Settings", cx - 220, cy - 100, 440, 340);
    if (!win) return;

    compp_label_create(win, 10, 6,  "YuguoOS Settings Panel");
    compp_widget_set_color(
        compp_label_create(win, 10, 24, "--------------------------------"),
        COLOR_DARK_GREY);

    compp_label_create(win, 10, 48, "Wallpaper:    Blue");
    compp_label_create(win, 10, 68, "Color Scheme: Windows Classic");
    compp_label_create(win, 10, 88, "Mouse Speed:  Medium");

    compp_widget_t *fx = compp_checkbox_create(win, 10, 112,
        "Enable Window Effects", fx_enabled);
    compp_widget_on_change(fx, on_fx_toggle);

    compp_groupbox_create(win, 10, 142, 250, 110, "System");

    compp_widget_t *vmem = compp_checkbox_create(win, 22, 172,
        "Enable Virtual Memory", vmem_enabled);
    compp_widget_on_change(vmem, on_vmem_toggle);

    compp_label_create(win, 22, 200, "Hostname:");
    compp_widget_t *host_box = compp_textbox_create(win, 100, 198, 140, hostname);
    compp_widget_on_change(host_box, on_hostname_change);

    compp_widget_t *apply_btn = compp_button_create(win, 10, 270, 80, 24, "Apply");
    compp_widget_on_click(apply_btn, on_apply);

    compp_widget_t *cancel_btn = compp_button_create(win, 100, 270, 80, 24, "Cancel");
    compp_widget_on_click(cancel_btn, on_cancel);

    compp_widget_t *about_btn = compp_button_create(win, 190, 270, 80, 24, "About");
    compp_widget_on_click(about_btn, on_about);

    serial_write("[APP] Settings started\n");
}