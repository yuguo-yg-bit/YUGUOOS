#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "proc.h"
#include "sched.h"
#include "syscall.h"
#include "vfs.h"
#include "vmm.h"
#include "timer.h"
#include "fb.h"
#include "kb.h"
#include "serial.h"

void kernel_init(uint32_t magic, uint32_t multiboot_addr);
void kernel_run(void);

int  kproc_create(void (*entry)(void), const char *name);
void kproc_exit(int code);
int  kproc_get_pid(void);
void kproc_yield(void);
void kproc_sleep_ms(uint32_t ms);
void kproc_print_list(void);

int  kopen(const char *path, int flags);
int  kclose(int fd);
int  kread(int fd, void *buf, size_t count);
int  kwrite(int fd, const void *buf, size_t count);

uint64_t ktime_ms(void);
void     ksleep_ms(uint32_t ms);

void kdebug(const char *msg);
void kdebug_hex(uint64_t value);
void kdebug_dec(uint64_t value);