/*
 * Copyright (C) 2020-2022 The opuntiaOS Project Authors.
 *  + Contributed by Nikita Melekhin <nimelehin@gmail.com>
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <drivers/debug/screen.h>
#include <drivers/serial/uart_api.h>
#include <libkern/libkern.h>
#include <libkern/lock.h>
#include <libkern/log.h>
#include <libkern/stdarg.h>
#include <libkern/types.h>
#include <mem/boot.h>

// Turn off lock debug output for log.
#ifdef DEBUG_SPINLOCK
#undef spinlock_acquire
#undef spinlock_release
#endif

static spinlock_t _log_lock;

static int putch_callback_stream(char c, char* buf_base, size_t* written, void* callback_params)
{
    return (uart_write(c) && screen_put_char(c));
}

static int vlog_unfmt(const char* format, va_list arg)
{
    return printf_engine(NULL, format, putch_callback_stream, NULL, arg);
}

static int vlog_fmt(const char* init_msg, const char* format, va_list arg)
{
    vlog_unfmt(init_msg, arg);
    vlog_unfmt(format, arg);

    size_t formatlen = strlen(format);
    if (format[formatlen - 1] != '\n') {
        vlog_unfmt("\n", arg);
    }
    return 0;
}

int log(const char* format, ...)
{
    spinlock_acquire(&_log_lock);
    va_list arg;
    va_start(arg, format);
    int ret = vlog_fmt("[LOG]  ", format, arg);
    va_end(arg);
    spinlock_release(&_log_lock);
    return ret;
}

int log_warn(const char* format, ...)
{
    spinlock_acquire(&_log_lock);
    va_list arg;
    va_start(arg, format);
    int ret = vlog_fmt("[WARN] ", format, arg);
    va_end(arg);
    spinlock_release(&_log_lock);
    return ret;
}

int log_error(const char* format, ...)
{
    spinlock_acquire(&_log_lock);
    va_list arg;
    va_start(arg, format);
    int ret = vlog_fmt("[ERR]  ", format, arg);
    va_end(arg);
    spinlock_release(&_log_lock);
    return ret;
}

int log_not_formatted(const char* format, ...)
{
    spinlock_acquire(&_log_lock);
    va_list arg;
    va_start(arg, format);
    int ret = vlog_unfmt(format, arg);
    va_end(arg);
    spinlock_release(&_log_lock);
    return ret;
}

void logger_setup(boot_args_t* boot_args)
{
    spinlock_init(&_log_lock);
    uart_setup(boot_args);
    screen_setup(boot_args);
}