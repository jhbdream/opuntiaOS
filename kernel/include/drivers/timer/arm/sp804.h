/*
 * Copyright (C) 2020-2022 The opuntiaOS Project Authors.
 *  + Contributed by Nikita Melekhin <nimelehin@gmail.com>
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef _KERNEL_DRIVERS_TIMER_ARM_SP804_H
#define _KERNEL_DRIVERS_TIMER_ARM_SP804_H

#include <drivers/driver_manager.h>
#include <libkern/mask.h>
#include <libkern/types.h>
#include <time/time_manager.h>

#define SP804_CLK_HZ 1000000

// https://developer.arm.com/documentation/ddi0271/d/programmer-s-model/register-descriptions/control-register--timerxcontrol?lang=en
enum SP804ControlMasks {
    MASKDEFINE(SP804_ONE_SHOT, 0, 1),
    MASKDEFINE(SP804_32_BIT, 1, 1),
    MASKDEFINE(SP804_PRESCALE, 2, 2),
    MASKDEFINE(SP804_INTS_ENABLED, 5, 1),
    MASKDEFINE(SP804_PERIODIC, 6, 1),
    MASKDEFINE(SP804_ENABLE, 7, 1),
};

struct sp804_registers {
    uint32_t load;
    uint32_t value;
    uint32_t control;
    uint32_t intclr;
    uint32_t ris;
    uint32_t mis;
    uint32_t bg_load;
};
typedef struct sp804_registers sp804_registers_t;

void sp804_install();

#endif //_KERNEL_DRIVERS_TIMER_ARM_SP804_H
