/*
 * Copyright (C) 2020-2022 The opuntiaOS Project Authors.
 *  + Contributed by Nikita Melekhin <nimelehin@gmail.com>
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef _KERNEL_DRIVERS_CLOCK_ARM_PL031_H
#define _KERNEL_DRIVERS_CLOCK_ARM_PL031_H

#include <drivers/driver_manager.h>
#include <libkern/mask.h>
#include <libkern/types.h>
#include <platform/arm32/interrupts.h>
#include <platform/arm32/target/cortex-a15/device_settings.h>

struct pl031_registers {
    uint32_t data;
    uint32_t match;
    uint32_t load;
    uint32_t control;
    // TO BE CONTINUED
};
typedef struct pl031_registers pl031_registers_t;

uint32_t pl031_read_rtc();

#endif // _KERNEL_DRIVERS_CLOCK_ARM_PL031_H