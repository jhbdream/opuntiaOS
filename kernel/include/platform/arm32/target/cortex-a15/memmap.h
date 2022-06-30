/*
 * Copyright (C) 2020-2022 The opuntiaOS Project Authors.
 *  + Contributed by Nikita Melekhin <nimelehin@gmail.com>
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef _KERNEL_PLATFORM_ARM32_TARGET_CORTEX_A15_MEMMAP_H
#define _KERNEL_PLATFORM_ARM32_TARGET_CORTEX_A15_MEMMAP_H

#define KMALLOC_BASE (KERNEL_BASE + 0x400000)

extern struct memory_map* arm_memmap;

#endif /* _KERNEL_PLATFORM_ARM32_TARGET_CORTEX_A15_MEMMAP_H */