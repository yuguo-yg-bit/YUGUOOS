#include "vmm.h"
#include "pmm.h"
#include "serial.h"

#define MIN_ALLOC 16
#define BLOCK_HEADER_SIZE 32

typedef struct block {
    size_t size;
    int    free;
    struct block *next;
    struct block *prev;
} block_t;

static block_t *heap_head = NULL;

static size_t align_up(size_t n, size_t align)
{
    return (n + align - 1) & ~(align - 1);
}

void vmm_init(void)
{
    heap_head = NULL;
    serial_write("[VMM] 内核堆分配器初始化完成\n");
}

void *kmalloc(size_t size)
{
    if (size == 0) return NULL;

    size_t total = align_up(size, 16);

    if (total + BLOCK_HEADER_SIZE > PAGE_SIZE) {
        size_t pages = (total + BLOCK_HEADER_SIZE + PAGE_SIZE - 1) / PAGE_SIZE;
        void *ptr = pmm_alloc_pages(pages);
        if (ptr) {
            block_t *blk = (block_t *)ptr;
            blk->size = pages * PAGE_SIZE;
            blk->free = 0;
            blk->next = NULL;
            blk->prev = NULL;
            return (void *)((uint8_t *)ptr + BLOCK_HEADER_SIZE);
        }
        return NULL;
    }

    block_t *cur = heap_head;
    while (cur) {
        if (cur->free && cur->size >= total) {
            if (cur->size >= total + BLOCK_HEADER_SIZE + MIN_ALLOC) {
                block_t *new_blk = (block_t *)((uint8_t *)cur + BLOCK_HEADER_SIZE + total);
                new_blk->size = cur->size - total - BLOCK_HEADER_SIZE;
                new_blk->free = 1;
                new_blk->next = cur->next;
                new_blk->prev = cur;
                if (cur->next) cur->next->prev = new_blk;
                cur->next = new_blk;
                cur->size = total;
            }
            cur->free = 0;
            return (void *)((uint8_t *)cur + BLOCK_HEADER_SIZE);
        }
        cur = cur->next;
    }

    void *page = pmm_alloc_page();
    if (!page) return NULL;

    block_t *blk = (block_t *)page;
    blk->size = PAGE_SIZE - BLOCK_HEADER_SIZE;
    blk->free = 0;
    blk->next = heap_head;
    blk->prev = NULL;
    if (heap_head) heap_head->prev = blk;
    heap_head = blk;

    if (blk->size >= total + BLOCK_HEADER_SIZE + MIN_ALLOC) {
        block_t *new_blk = (block_t *)((uint8_t *)blk + BLOCK_HEADER_SIZE + total);
        new_blk->size = blk->size - total - BLOCK_HEADER_SIZE;
        new_blk->free = 1;
        new_blk->next = blk->next;
        new_blk->prev = blk;
        if (blk->next) blk->next->prev = new_blk;
        blk->next = new_blk;
        blk->size = total;
    }

    return (void *)((uint8_t *)blk + BLOCK_HEADER_SIZE);
}

void kfree(void *ptr)
{
    if (!ptr) return;

    block_t *blk = (block_t *)((uint8_t *)ptr - BLOCK_HEADER_SIZE);

    if (blk->size >= PAGE_SIZE) {
        pmm_free_pages(blk, blk->size / PAGE_SIZE);
        return;
    }

    blk->free = 1;

    if (blk->next && blk->next->free) {
        blk->size += BLOCK_HEADER_SIZE + blk->next->size;
        blk->next = blk->next->next;
        if (blk->next) blk->next->prev = blk;
    }

    if (blk->prev && blk->prev->free) {
        blk->prev->size += BLOCK_HEADER_SIZE + blk->size;
        blk->prev->next = blk->next;
        if (blk->next) blk->next->prev = blk->prev;
    }
}

void *krealloc(void *ptr, size_t new_size)
{
    if (!ptr) return kmalloc(new_size);
    if (new_size == 0) { kfree(ptr); return NULL; }

    block_t *blk = (block_t *)((uint8_t *)ptr - BLOCK_HEADER_SIZE);
    size_t total = align_up(new_size, 16);

    if (total <= blk->size) {
        return ptr;
    }

    void *new_ptr = kmalloc(new_size);
    if (!new_ptr) return NULL;

    size_t copy_size = blk->size;
    if (copy_size > new_size) copy_size = new_size;
    for (size_t i = 0; i < copy_size; i++) {
        ((uint8_t *)new_ptr)[i] = ((uint8_t *)ptr)[i];
    }
    kfree(ptr);
    return new_ptr;
}

void heap_print_stats(void)
{
    serial_write("[堆] 空闲块: ");
    block_t *cur = heap_head;
    size_t count = 0;
    size_t free_bytes = 0;
    while (cur) {
        count++;
        if (cur->free) free_bytes += cur->size;
        cur = cur->next;
    }
    serial_write_dec(count);
    serial_write("  空闲字节: ");
    serial_write_dec(free_bytes);
    serial_write("\n");
}