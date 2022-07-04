/*
 * Copyright (C) 2020-2022 The opuntiaOS Project Authors.
 *  + Contributed by Nikita Melekhin <nimelehin@gmail.com>
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef _KERNEL_PLATFORM_X86_SYSTEM_H
#define _KERNEL_PLATFORM_X86_SYSTEM_H

#include <libkern/c_attrs.h>
#include <libkern/types.h>
#include <platform/generic/registers.h>

/**
 * INTS
 */

void system_disable_interrupts();
void system_enable_interrupts();
void system_enable_interrupts_only_counter();
inline static void system_disable_interrupts_no_counter() { asm volatile("cli"); }
inline static void system_enable_interrupts_no_counter() { asm volatile("sti"); }

/**
 * PAGING
 */

inline static void system_set_pdir(uintptr_t pdir0, uintptr_t pdir1)
{
    asm volatile("mov %0, %%cr3"
                 :
                 : "r"(pdir0)
                 : "memory");
}

inline static void system_flush_local_tlb_entry(uintptr_t vaddr)
{
    asm volatile("invlpg (%0)" ::"r"(vaddr)
                 : "memory");
}

inline static void system_flush_all_cpus_tlb_entry(uintptr_t vaddr)
{
    system_flush_local_tlb_entry(vaddr);
    // TODO: Send inter-processor messages.
}

inline static void system_flush_whole_tlb()
{
    system_set_pdir(read_cr3(), 0x0);
}

inline static void system_enable_write_protect()
{
    uintptr_t cr = read_cr0();
    cr |= 0x10000;
    write_cr0(cr);
}

inline static void system_disable_write_protect()
{
    uintptr_t cr = read_cr0();
    cr &= 0xfffeffff;
    write_cr0(cr);
}

inline static void system_enable_paging()
{
    uintptr_t cr = read_cr0();
    cr |= 0x80000000;
    write_cr0(cr);
}

inline static void system_disable_paging()
{
    uintptr_t cr = read_cr0();
    cr &= 0x7fffffff;
    write_cr0(cr);
}

inline static void system_stop_until_interrupt()
{
    asm volatile("hlt");
}

NORETURN inline static void system_stop()
{
    system_disable_interrupts();
    system_stop_until_interrupt();
    while (1) { }
}

/**
 * CPU
 */

void system_cache_clean_and_invalidate(void* addr, size_t size);
void system_cache_invalidate(void* addr, size_t size);
void system_cache_clean(void* addr, size_t size);

inline static int system_cpu_id()
{
    return 0;
}

#endif /* _KERNEL_PLATFORM_X86_SYSTEM_H */