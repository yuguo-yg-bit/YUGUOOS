#include "pmm.h"
#include "serial.h"
#include <stdbool.h>

struct multiboot2_tag {
    uint32_t type;
    uint32_t size;
} __attribute__((packed));

struct multiboot2_mmap_entry {
    uint64_t base_addr;
    uint64_t length;
    uint32_t type;
    uint32_t reserved;
} __attribute__((packed));

#define MULTIBOOT2_TAG_MMAP  6
#define MULTIBOOT2_MMAP_FREE 1

extern uint8_t _kernel_end[];

static uint8_t  *bitmap;
static uint64_t  mem_total_pages;
static uint64_t  mem_free_pages;
static uint64_t  mem_usable_start;
static uint64_t  mem_usable_end;

static inline void bitmap_set(uint64_t page)
{
    bitmap[page / 8] |= (1 << (page % 8));
}

static inline void bitmap_clear(uint64_t page)
{
    bitmap[page / 8] &= ~(1 << (page % 8));
}

static inline bool bitmap_test(uint64_t page)
{
    return (bitmap[page / 8] & (1 << (page % 8))) != 0;
}

void pmm_init(uintptr_t multiboot_info_addr)
{
    uint64_t highest_addr = 0;
    uint64_t usable_start = UINT64_MAX;
    uint64_t usable_end   = 0;

    struct multiboot2_tag *tag = (struct multiboot2_tag *)(uintptr_t)(multiboot_info_addr + 8);

    while (tag->type != 0) {
        if (tag->type == MULTIBOOT2_TAG_MMAP) {
            uint32_t entry_size = ((uint32_t *)tag)[2];
            struct multiboot2_mmap_entry *entry =
                (struct multiboot2_mmap_entry *)((uint8_t *)tag + 16);
            uint32_t count = (tag->size - 16) / entry_size;

            for (uint32_t i = 0; i < count; i++) {
                struct multiboot2_mmap_entry *cur =
                    (struct multiboot2_mmap_entry *)((uint8_t *)entry + i * entry_size);

                uint64_t end = cur->base_addr + cur->length;
                if (end > highest_addr) {
                    highest_addr = end;
                }

                if (cur->type == MULTIBOOT2_MMAP_FREE) {
                    if (cur->base_addr < usable_start) {
                        usable_start = cur->base_addr;
                    }
                    if (end > usable_end) {
                        usable_end = end;
                    }
                }
            }
        }
        tag = (struct multiboot2_tag *)((uint8_t *)tag + ((tag->size + 7) & ~7));
    }

    mem_total_pages  = highest_addr / PAGE_SIZE;
    mem_usable_start = usable_start;
    mem_usable_end   = usable_end;

    size_t bitmap_size = (mem_total_pages + 7) / 8;
    uint64_t kernel_phys_end = ((uint64_t)_kernel_end + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

    bitmap = (uint8_t *)kernel_phys_end;

    for (size_t i = 0; i < bitmap_size; i++) {
        bitmap[i] = 0xFF;
    }

    mem_free_pages = 0;

    tag = (struct multiboot2_tag *)(uintptr_t)(multiboot_info_addr + 8);
    while (tag->type != 0) {
        if (tag->type == MULTIBOOT2_TAG_MMAP) {
            uint32_t entry_size = ((uint32_t *)tag)[2];
            struct multiboot2_mmap_entry *entry =
                (struct multiboot2_mmap_entry *)((uint8_t *)tag + 16);
            uint32_t count = (tag->size - 16) / entry_size;

            for (uint32_t i = 0; i < count; i++) {
                struct multiboot2_mmap_entry *cur =
                    (struct multiboot2_mmap_entry *)((uint8_t *)entry + i * entry_size);

                if (cur->type != MULTIBOOT2_MMAP_FREE) continue;

                uint64_t start = cur->base_addr;
                uint64_t end   = cur->base_addr + cur->length;

                if (start < 0x100000) start = 0x100000;
                if (start >= end) continue;

                start = (start + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
                end   = end & ~(PAGE_SIZE - 1);

                for (uint64_t addr = start; addr < end; addr += PAGE_SIZE) {
                    uint64_t page = addr / PAGE_SIZE;
                    if (bitmap_test(page)) {
                        bitmap_clear(page);
                        mem_free_pages++;
                    }
                }
            }
        }
        tag = (struct multiboot2_tag *)((uint8_t *)tag + ((tag->size + 7) & ~7));
    }

    for (uint64_t addr = 0x100000; addr < kernel_phys_end; addr += PAGE_SIZE) {
        uint64_t page = addr / PAGE_SIZE;
        if (!bitmap_test(page)) {
            bitmap_set(page);
            mem_free_pages--;
        }
    }

    uint64_t bitmap_pages = (bitmap_size + kernel_phys_end + PAGE_SIZE - 1) / PAGE_SIZE;
    for (uint64_t addr = kernel_phys_end; addr < kernel_phys_end + bitmap_pages * PAGE_SIZE; addr += PAGE_SIZE) {
        uint64_t page = addr / PAGE_SIZE;
        if (!bitmap_test(page)) {
            bitmap_set(page);
            mem_free_pages--;
        }
    }

    serial_write("[PMM] 物理内存管理初始化完成\n");
    serial_write("  总页面: ");
    serial_write_hex(mem_total_pages);
    serial_write("  可用页面: ");
    serial_write_hex(mem_free_pages);
    serial_write("  位图大小: ");
    serial_write_hex(bitmap_size);
    serial_write(" 字节\n");
    serial_write("  可用内存: ");
    serial_write_hex(usable_start);
    serial_write(" - ");
    serial_write_hex(usable_end);
    serial_write("\n");
}

void *pmm_alloc_page(void)
{
    for (uint64_t i = 0; i < mem_total_pages; i++) {
        if (!bitmap_test(i)) {
            bitmap_set(i);
            mem_free_pages--;
            return (void *)(uintptr_t)(i * PAGE_SIZE);
        }
    }
    return (void *)0;
}

void *pmm_alloc_pages(size_t count)
{
    if (count == 0) return (void *)0;

    uint64_t run_start = 0;
    uint64_t run_len   = 0;

    for (uint64_t i = 0; i < mem_total_pages; i++) {
        if (!bitmap_test(i)) {
            if (run_len == 0) run_start = i;
            run_len++;
            if (run_len >= count) {
                for (uint64_t j = run_start; j < run_start + count; j++) {
                    bitmap_set(j);
                }
                mem_free_pages -= count;
                return (void *)(uintptr_t)(run_start * PAGE_SIZE);
            }
        } else {
            run_len = 0;
        }
    }
    return (void *)0;
}

void pmm_free_page(void *addr)
{
    uint64_t page = (uint64_t)(uintptr_t)addr / PAGE_SIZE;
    if (page >= mem_total_pages) return;
    if (bitmap_test(page)) {
        bitmap_clear(page);
        mem_free_pages++;
    }
}

void pmm_free_pages(void *addr, size_t count)
{
    for (size_t i = 0; i < count; i++) {
        pmm_free_page((void *)((uintptr_t)addr + i * PAGE_SIZE));
    }
}

size_t pmm_total_pages(void)
{
    return (size_t)mem_total_pages;
}

size_t pmm_get_free_pages(void)
{
    return (size_t)mem_free_pages;
}

void pmm_print_stats(void)
{
    serial_write("物理内存: 总计 ");
    serial_write_hex(mem_total_pages * PAGE_SIZE / 1024);
    serial_write(" KB, 可用 ");
    serial_write_hex(mem_free_pages * PAGE_SIZE / 1024);
    serial_write(" KB\n");
}