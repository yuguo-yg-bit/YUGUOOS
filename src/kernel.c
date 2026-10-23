#include "kernel.h"
#include "kernel_api.h"
#include "hal.h"
#include "vga.h"
#include "serial.h"
#include "gdt.h"
#include "idt.h"
#include "pic.h"
#include "pmm.h"
#include "vmm.h"
#include "irq.h"
#include "timer.h"
#include "proc.h"
#include "sched.h"
#include "syscall.h"
#include "vfs.h"
#include "fb.h"
#include "font.h"
#include "kb.h"

static const char *spinner_chars = "|/-\\";

static void fb_boot_animation(void)
{
    if (!fb_available()) return;

    fb_clear(FB_BLACK);

    uint32_t sw = fb_get_width();
    uint32_t sh = fb_get_height();

    for (int y = 0; y < 6; y++) {
        uint32_t cy = sh / 2 - 180 + y;
        uint32_t alpha = 30 + y * 30;
        uint32_t color = FB_COLOR(0, alpha, alpha + 40);
        fb_fill_rect(0, (int32_t)cy - 2, sw, 6, color);
    }

    fb_draw_huge_string_centered(sh / 2 - 140, "YUGUO OS", FB_CYAN);

    uint32_t title_y = sh / 2 - 60;
    fb_draw_big_string_centered(title_y, "v0.2.0 图形模式", FB_LIGHT_GREY);

    const char *subsystems[] = {
        "串口驱动 (COM1)",
        "全局描述符表 (GDT)",
        "中断描述符表 (IDT, 256条目)",
        "可编程中断控制器 (PIC)",
        "物理内存管理器 (PMM)",
        "键盘驱动 (PS/2 IRQ1)",
        "帧缓冲图形 (FB)",
        NULL
    };

    uint32_t init_y = title_y + 50;
    int spinner_idx = 0;

    for (int i = 0; subsystems[i] != NULL; i++) {
        char buf[64];
        int pos = 0;
        while (pos < 63 && subsystems[i][pos]) {
            buf[pos] = subsystems[i][pos];
            pos++;
        }
        buf[pos] = '\0';

        for (int spin = 0; spin < 6; spin++) {
            char line[80];
            int lp = 0;
            line[lp++] = ' ';
            line[lp++] = ' ';
            line[lp++] = '[';
            line[lp++] = spinner_chars[spinner_idx];
            line[lp++] = ']';
            line[lp++] = ' ';
            for (int k = 0; buf[k]; k++) {
                line[lp++] = buf[k];
            }
            line[lp] = '\0';

            uint32_t sx = (sw - (uint32_t)lp * FB_BIG_CHAR_W) / 2;
            fb_fill_rect(sx, init_y + (uint32_t)i * 40,
                         (uint32_t)lp * FB_BIG_CHAR_W, FB_BIG_CHAR_H, FB_BLACK);
            fb_draw_big_string(sx, init_y + (uint32_t)i * 40, line, FB_GREEN);
            spinner_idx = (spinner_idx + 1) % 4;

            for (volatile int d = 0; d < 500000; d++) {
                asm volatile("pause");
            }
        }

        char done_line[80];
        int dlp = 0;
        done_line[dlp++] = ' ';
        done_line[dlp++] = ' ';
        done_line[dlp++] = '[';
        done_line[dlp++] = 'O';
        done_line[dlp++] = 'K';
        done_line[dlp++] = ']';
        done_line[dlp++] = ' ';
        for (int k = 0; buf[k]; k++) {
            done_line[dlp++] = buf[k];
        }
        done_line[dlp] = '\0';
        uint32_t dsx = (sw - (uint32_t)dlp * FB_BIG_CHAR_W) / 2;
        fb_fill_rect(dsx, init_y + (uint32_t)i * 40,
                     (uint32_t)dlp * FB_BIG_CHAR_W, FB_BIG_CHAR_H, FB_BLACK);
        fb_draw_big_string(dsx, init_y + (uint32_t)i * 40, done_line, FB_DARK_GREEN);
    }
}

static void fb_login_screen(void)
{
    if (!fb_available()) return;

    uint32_t sw = fb_get_width();
    uint32_t sh = fb_get_height();

    fb_clear(FB_BLACK);

    for (int y = 0; y < 5; y++) {
        uint32_t cy = 100 + y;
        uint32_t alpha = 20 + y * 30;
        uint32_t color = FB_COLOR(0, alpha, alpha + 30);
        fb_fill_rect(0, (int32_t)cy - 1, sw, 5, color);
    }

    fb_draw_huge_string_centered(150, "YUGUO OS", FB_CYAN);

    uint32_t box_w = 800;
    uint32_t box_h = 520;
    uint32_t box_x = (sw - box_w) / 2;
    uint32_t box_y = 300;

    fb_draw_rect(box_x, box_y, box_w, box_h, FB_DARK_BLUE);
    fb_fill_rect(box_x + 1, box_y + 1, box_w - 2, 60, FB_COLOR(0, 0, 40));

    fb_draw_big_string_centered(box_y + 14, "系 统 登 录", FB_WHITE);

    uint32_t user_field_y = box_y + 120;
    fb_draw_big_string(box_x + 60, user_field_y, "用户名:", FB_LIGHT_GREY);
    fb_draw_rect(box_x + 220, user_field_y - 4, 460, 40, FB_MID_GREY);

    uint32_t pass_field_y = user_field_y + 100;
    fb_draw_big_string(box_x + 60, pass_field_y, "密  码:", FB_LIGHT_GREY);
    fb_draw_rect(box_x + 220, pass_field_y - 4, 460, 40, FB_MID_GREY);

    uint32_t btn_w = 240;
    uint32_t btn_h = 60;
    uint32_t btn_x = box_x + (box_w - btn_w) / 2;
    uint32_t btn_y = box_y + 340;
    fb_draw_rect(btn_x, btn_y, btn_w, btn_h, FB_CYAN);
    fb_fill_rect(btn_x + 1, btn_y + 1, btn_w - 2, btn_h - 2, FB_COLOR(0, 40, 60));
    fb_draw_big_string_centered(btn_y + 14, "登  录", FB_WHITE);

    fb_draw_big_string_centered(sh - 80, "YuguoOS v0.2.0 | Clang + LLD | x86_64", FB_DARK_GREY);

    vga_set_color(VGA_WHITE, VGA_BLACK);
    vga_writeln("\n图形登录界面已就绪！在终端按 Enter 进入。");

    char username[64] = {0};
    int user_pos = 0;
    char password[64] = {0};
    int pass_pos = 0;
    int field = 0;

    uint32_t cursor_timer = 0;

    for (;;) {
        if (kb_haschar()) {
            char c = kb_getchar();

            if (c == '\n') {
                if (field == 0) {
                    field = 1;
                } else {
                    break;
                }
            } else if (c == '\b') {
                if (field == 0 && user_pos > 0) {
                    user_pos--;
                    username[user_pos] = '\0';
                    uint32_t cx = box_x + 220 + (uint32_t)user_pos * FB_BIG_CHAR_W;
                    fb_fill_rect(cx, user_field_y, FB_BIG_CHAR_W, FB_BIG_CHAR_H, FB_BLACK);
                } else if (field == 1 && pass_pos > 0) {
                    pass_pos--;
                    password[pass_pos] = '\0';
                    uint32_t cx = box_x + 220 + (uint32_t)pass_pos * FB_BIG_CHAR_W;
                    fb_fill_rect(cx, pass_field_y, FB_BIG_CHAR_W, FB_BIG_CHAR_H, FB_BLACK);
                }
            } else if (c >= 32 && c <= 126) {
                if (field == 0 && user_pos < 25) {
                    username[user_pos++] = c;
                    username[user_pos] = '\0';
                    uint32_t cx = box_x + 220 + (uint32_t)(user_pos - 1) * FB_BIG_CHAR_W;
                    fb_draw_big_char(cx, user_field_y, c, FB_WHITE);
                } else if (field == 1 && pass_pos < 25) {
                    password[pass_pos++] = c;
                    password[pass_pos] = '\0';
                    uint32_t cx = box_x + 220 + (uint32_t)(pass_pos - 1) * FB_BIG_CHAR_W;
                    fb_draw_big_char(cx, pass_field_y, '*', FB_WHITE);
                }
            }
        }

        cursor_timer++;
        if (cursor_timer % 600000 == 0) {
            int visible = (cursor_timer / 600000) % 2;
            uint32_t cur_y = user_field_y;
            uint32_t cur_x_in = box_x + 220;
            int cur_pos = user_pos;
            if (field == 1) {
                cur_y = pass_field_y;
                cur_pos = pass_pos;
            }
            uint32_t cur_color = visible ? FB_WHITE : FB_BLACK;
            uint32_t cx = cur_x_in + (uint32_t)cur_pos * FB_BIG_CHAR_W;
            for (int cr = 0; cr < FB_BIG_CHAR_H; cr++) {
                for (int cc = 0; cc < FB_BIG_CHAR_W; cc++) {
                    fb_putpixel(cx + cc, cur_y + cr, cur_color);
                }
            }
        }

        asm volatile("hlt");
    }

    fb_clear(FB_BLACK);
    fb_draw_huge_string_centered(sh / 2 - 120, "YUGUO OS", FB_CYAN);
    fb_draw_big_string_centered(sh / 2 - 40, "欢迎回来!", FB_GREEN);
    fb_draw_big_string_centered(sh / 2 + 20, "系统已成功启动", FB_LIGHT_GREY);

    for (;;) {
        asm volatile("hlt");
    }
}

void kernel_main(uint32_t magic, uint32_t multiboot_addr)
{
    (void)magic;

    serial_init();
    serial_write("===== YuguoOS v0.3.0 =====\n");

    vga_init();
    vga_clear();
    vga_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
    vga_writeln("YuguoOS v0.3.0 启动中...");
    vga_set_color(VGA_WHITE, VGA_BLACK);

    serial_write("[INIT] 第1层: 启动汇编已完成\n");
    serial_write("[INIT] 第2层: 最小C运行时\n");

    gdt_init();
    serial_write("[INIT] 第3层: HAL 硬件抽象层\n");
    idt_init();

    pic_remap();
    pic_mask_all();

    pmm_init(multiboot_addr);
    vmm_init();
    serial_write("[INIT] 第4层: 内存管理 (PMM + VMM)\n");

    irq_init();
    timer_init(100);
    serial_write("[INIT] 第5层: 中断 & 定时器\n");

    proc_init();
    sched_init();
    sched_start();
    serial_write("[INIT] 第6层: 进程 & 调度器\n");

    syscall_init();
    serial_write("[INIT] 第7层: 系统调用\n");

    vfs_init();
    serial_write("[INIT] 第8层: VFS 虚拟文件系统\n");

    kb_init();
    sti();

    fb_init(multiboot_addr);

    serial_write("开始启动动画...\n");
    fb_boot_animation();
    fb_login_screen();

    serial_write("系统进入空闲循环\n");
    for (;;) {
        asm volatile("hlt");
    }
}