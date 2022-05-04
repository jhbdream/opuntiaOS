/*
 * Copyright (C) 2020-2022 The opuntiaOS Project Authors.
 *  + Contributed by Nikita Melekhin <nimelehin@gmail.com>
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <algo/bitmap.h>
#include <libkern/libkern.h>
#include <libkern/lock.h>
#include <libkern/log.h>
#include <mem/kmalloc.h>
#include <mem/kmemzone.h>

struct kmalloc_header {
    size_t len;
};
typedef struct kmalloc_header kmalloc_header_t;

static spinlock_t _kmalloc_lock;
static kmemzone_t _kmalloc_zone;
static size_t _kmalloc_bitmap_len = 0;
static uint8_t* _kmalloc_bitmap;
static bitmap_t bitmap;

static inline uintptr_t kmalloc_to_vaddr(int start)
{
    uintptr_t vaddr = (uintptr_t)_kmalloc_zone.start + start * KMALLOC_BLOCK_SIZE;
    return (uintptr_t)_kmalloc_zone.start + start * KMALLOC_BLOCK_SIZE;
}

static inline int kmalloc_to_index(uintptr_t vaddr)
{
    return (vaddr - (uintptr_t)_kmalloc_zone.start) / KMALLOC_BLOCK_SIZE;
}

static void _kmalloc_init_bitmap()
{
    _kmalloc_bitmap = (uint8_t*)_kmalloc_zone.start;
    _kmalloc_bitmap_len = (KMALLOC_SPACE_SIZE / KMALLOC_BLOCK_SIZE / 8);
    vmm_ensure_writing_to_active_address_space((uintptr_t)_kmalloc_bitmap, _kmalloc_bitmap_len);

    bitmap = bitmap_wrap(_kmalloc_bitmap, _kmalloc_bitmap_len);
    memset(_kmalloc_bitmap, 0, _kmalloc_bitmap_len);

    /* Setting bitmap as a busy region. */
    int blocks_needed = (_kmalloc_bitmap_len + KMALLOC_BLOCK_SIZE - 1) / KMALLOC_BLOCK_SIZE;
    bitmap_set_range(bitmap, kmalloc_to_index((uintptr_t)_kmalloc_bitmap), blocks_needed);
}

void kmalloc_init()
{
    spinlock_init(&_kmalloc_lock);
    _kmalloc_zone = kmemzone_new(KMALLOC_SPACE_SIZE);
    _kmalloc_init_bitmap();
}

void* kmalloc(size_t size)
{
    spinlock_acquire(&_kmalloc_lock);
    int act_size = size + sizeof(kmalloc_header_t);

    int blocks_needed = (act_size + KMALLOC_BLOCK_SIZE - 1) / KMALLOC_BLOCK_SIZE;

    int start = bitmap_find_space(bitmap, blocks_needed);
    if (start < 0) {
        log_error("NO SPACE AT KMALLOC");
        system_stop();
    }

    kmalloc_header_t* space = (kmalloc_header_t*)kmalloc_to_vaddr(start);
    bitmap_set_range(bitmap, start, blocks_needed);
    spinlock_release(&_kmalloc_lock);

    vmm_ensure_writing_to_active_address_space((uintptr_t)space, act_size);
    space->len = act_size;
    return (void*)&space[1];
}

void* kmalloc_aligned(size_t size, size_t alignment)
{
    void* ptr = kmalloc(size + alignment + sizeof(void*));
    size_t max_addr = (size_t)ptr + sizeof(void*) + alignment;
    void* aligned_ptr = (void*)(max_addr - (max_addr % alignment));
    ((void**)aligned_ptr)[-1] = ptr;
    return aligned_ptr;
}

void* kmalloc_page_aligned()
{
    return kmalloc_aligned(VMM_PAGE_SIZE, VMM_PAGE_SIZE);
}

void kfree(void* ptr)
{
    if (!ptr) {
        DEBUG_ASSERT("NULL at kfree");
        return;
    }

    kmalloc_header_t* sptr = (kmalloc_header_t*)ptr;
    int blocks_to_delete = (sptr[-1].len + KMALLOC_BLOCK_SIZE - 1) / KMALLOC_BLOCK_SIZE;
    spinlock_acquire(&_kmalloc_lock);
    bitmap_unset_range(bitmap, kmalloc_to_index((size_t)&sptr[-1]), blocks_to_delete);
    spinlock_release(&_kmalloc_lock);
}

void kfree_aligned(void* ptr)
{
    if (!ptr) {
        DEBUG_ASSERT("NULL at kfree_aligned");
        return;
    }

    kfree(((void**)ptr)[-1]);
}

void* krealloc(void* ptr, size_t new_size)
{
    size_t old_size = ((kmalloc_header_t*)ptr)[-1].len;
    if (old_size == new_size) {
        return ptr;
    }

    uint8_t* new_area = kmalloc(new_size);
    if (new_area == 0) {
        return 0;
    }

    memcpy(new_area, ptr, new_size > old_size ? new_size : old_size);
    kfree(ptr);

    return new_area;
}