/*
 * Copyright (C) 2020-2022 The opuntiaOS Project Authors.
 *  + Contributed by Nikita Melekhin <nimelehin@gmail.com>
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef _KERNEL_PLATFORM_ARM64_TASKING_CONTEXT_H
#define _KERNEL_PLATFORM_ARM64_TASKING_CONTEXT_H

#include <libkern/c_attrs.h>
#include <libkern/types.h>

typedef struct {
    uint64_t x[22];
    uint64_t lr;
} PACKED context_t;

static inline uintptr_t context_get_instruction_pointer(context_t* ctx)
{
    return ctx->lr;
}

static inline void context_set_instruction_pointer(context_t* ctx, uintptr_t ip)
{
    ctx->lr = ip;
}

#endif // _KERNEL_PLATFORM_ARM64_TASKING_CONTEXT_H