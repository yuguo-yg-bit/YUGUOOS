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
#include "gui.h"

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

    serial_write("[INIT] 第9-15层: 启动桌面环境...\n");
    gui_start();
    gui_run();
}