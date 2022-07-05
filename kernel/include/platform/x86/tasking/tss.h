/*
 * Copyright (C) 2020-2022 The opuntiaOS Project Authors.
 *  + Contributed by Nikita Melekhin <nimelehin@gmail.com>
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef _KERNEL_PLATFORM_X86_TASKING_TSS_H
#define _KERNEL_PLATFORM_X86_TASKING_TSS_H

#include <libkern/c_attrs.h>
#include <libkern/types.h>

#define SEGTSS_TYPE 0x9 // defined in the Intel's manual 3a

#ifdef __i386__
struct PACKED tss {
    uint32_t back_link : 16; // back link to prev tss
    uint32_t zero1 : 16; // always zero
    uint32_t esp0 : 32; // stack pointer at ring 0
    uint32_t ss0 : 16; // stack segment at ring 0
    uint32_t zero2 : 16; // always zero
    uint32_t esp1 : 32; // stack pointer at ring 1
    uint32_t ss1 : 16; // stack segment at ring 1
    uint32_t zero3 : 16; // always zero
    uint32_t esp2 : 32; // stack pointer at ring 2
    uint32_t ss2 : 16; // stack segment at ring 2
    uint32_t zero4 : 16; // always zero
    uint32_t cr3 : 32;
    uint32_t eip : 32;
    uint32_t eflag : 32;
    uint32_t eax : 32;
    uint32_t ecx : 32;
    uint32_t edx : 32;
    uint32_t ebx : 32;
    uint32_t esp : 32;
    uint32_t ebp : 32;
    uint32_t esi : 32;
    uint32_t edi : 32;
    uint32_t es : 16;
    uint32_t zero5 : 16; // always zero
    uint32_t cs : 16;
    uint32_t zero6 : 16; // always zero
    uint32_t ss : 16;
    uint32_t zero7 : 16; // always zero
    uint32_t ds : 16;
    uint32_t zero8 : 16; // always zero
    uint32_t fs : 16;
    uint32_t zero9 : 16; // always zero
    uint32_t gs : 16;
    uint32_t zero10 : 16; // always zero
    uint32_t ldt_selector : 16;
    uint32_t zero11 : 16; // always zero
    uint32_t t : 1;
    uint32_t zero12 : 15; // always zero
    uint32_t iomap_offset : 16;
};
#elif __x86_64__
struct PACKED tss {
    uint32_t res1;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t res2;
    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;
    uint64_t res3;
    uint16_t res4;
    uint16_t iomap_offset;
};
#endif
typedef struct tss tss_t;

extern tss_t tss;

void set_ltr(uint16_t seg);

#endif //_KERNEL_PLATFORM_X86_TASKING_TSS_H