/*
 * Copyright (C) 2020-2022 The opuntiaOS Project Authors.
 *  + Contributed by Nikita Melekhin <nimelehin@gmail.com>
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef _KERNEL_IO_TTY_TTY_H
#define _KERNEL_IO_TTY_TTY_H

#include <algo/sync_ringbuffer.h>
#include <fs/vfs.h>
#include <libkern/types.h>

#define TTY_MAX_COUNT 8
#define TTY_BUFFER_SIZE 1024

typedef unsigned char cc_t;
typedef unsigned int speed_t;
typedef unsigned int tcflag_t;

#define NCCS 32
struct termios {
    tcflag_t c_iflag; // input mode flags
    tcflag_t c_oflag; // output mode flags
    tcflag_t c_cflag; // control mode flags
    tcflag_t c_lflag; // local mode flags
    cc_t c_cc[NCCS]; // control characters
};
typedef struct termios termios_t;

/* c_cc characters */
#define VINTR 0
#define VQUIT 1
#define VERASE 2
#define VKILL 3
#define VEOF 4
#define VTIME 5
#define VMIN 6
#define VSWTC 7
#define VSTART 8
#define VSTOP 9
#define VSUSP 10
#define VEOL 11
#define VREPRINT 12
#define VDISCARD 13
#define VWERASE 14
#define VLNEXT 15
#define VEOL2 16

/* c_lflag bits */
#define ISIG 0000001
#define ICANON 0000002
#define ECHO 0000010
#define ECHOE 0000020
#define ECHOK 0000040
#define ECHONL 0000100
#define NOFLSH 0000200
#define TOSTOP 0000400
#define IEXTEN 0100000

/* tcflow() and TCXONC use these */
#define TCOOFF 0
#define TCOON 1
#define TCIOFF 2
#define TCION 3

/* tcflush() and TCFLSH use these */
#define TCIFLUSH 0
#define TCOFLUSH 1
#define TCIOFLUSH 2

/* tcsetattr uses these */
#define TCSANOW 0
#define TCSADRAIN 1
#define TCSAFLUSH 2

struct tty_entry {
    sync_ringbuffer_t buffer;
    int line_count;
    gid_t pgid;
    termios_t termios;
};
typedef struct tty_entry tty_entry_t;

int tty_init(tty_entry_t* tty);
int tty_clear(tty_entry_t* tty);

bool tty_can_read(tty_entry_t* tty, file_t* file, size_t start);
int tty_read(tty_entry_t* tty, file_t* file, void __user* buf, size_t start, size_t len);
bool tty_can_write(tty_entry_t* tty, file_t* file, size_t start);
int tty_write(tty_entry_t* tty, file_t* file, void __user* buf, size_t start, size_t len);
int tty_ioctl(tty_entry_t* tty, file_t* file, uintptr_t cmd, uintptr_t arg);

#endif // _KERNEL_IO_TTY_TTY_H