#include "gui.h"
#include "app.h"
#include "compp.h"
#include "serial.h"

void gui_start(void)
{
    serial_write("[GUI] 初始化 COM++ 桌面环境...\n");

    compp_init();

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