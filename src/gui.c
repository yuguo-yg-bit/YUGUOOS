#include "gui.h"
#include "app.h"
#include "compp.h"
#include "desktop.h"
#include "serial.h"

static void open_desktop(void)  { app_desktop_run();  }
static void open_recycle(void)  { app_recycle_run();  }
static void open_settings(void) { app_settings_run(); }
static void open_terminal(void) { app_terminal_run(); }

void gui_start(void)
{
    serial_write("[GUI] 初始化 COM++ 桌面环境...\n");

    compp_init();

    serial_write("[GUI] 注册桌面图标...\n");

    desktop_add_icon(32,  32,  64, 52, "This PC",      open_desktop);
    desktop_add_icon(32,  96,  64, 52, "Recycle Bin",  open_recycle);
    desktop_add_icon(32,  160, 64, 52, "Settings",     open_settings);
    desktop_add_icon(32,  224, 64, 52, "Terminal",     open_terminal);

    serial_write("[GUI] 启动内置应用...\n");

    app_desktop_run();
    app_recycle_run();
    app_settings_run();
    app_terminal_run();

    serial_write("[GUI] 桌面环境就绪，窗口数: ");
    char buf[8];
    int cnt = compp_sys_window_count();
    int pos = 0;
    if (cnt >= 10) { buf[pos++] = (char)('0' + cnt / 10); }
    buf[pos++] = (char)('0' + cnt % 10);
    buf[pos] = '\0';
    serial_write(buf);
    serial_write("\n");
}

void gui_run(void)
{
    compp_run();
}