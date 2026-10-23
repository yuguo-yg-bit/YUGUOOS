#pragma once
#include <stdint.h>
#include <stddef.h>

#define PAGE_SIZE 4096

void pmm_init(uintptr_t multiboot_info_addr);
void *pmm_alloc_page(void);
void *pmm_alloc_pages(size_t count);
void pmm_free_page(void *addr);
void pmm_free_pages(void *addr, size_t count);
size_t pmm_total_pages(void);
size_t pmm_get_free_pages(void);
void pmm_print_stats(void);