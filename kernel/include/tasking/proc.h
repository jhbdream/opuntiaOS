/*
 * Copyright (C) 2020-2022 The opuntiaOS Project Authors.
 *  + Contributed by Nikita Melekhin <nimelehin@gmail.com>
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef _KERNEL_TASKING_PROC_H
#define _KERNEL_TASKING_PROC_H

#include <algo/dynamic_array.h>
#include <fs/vfs.h>
#include <io/tty/vconsole.h>
#include <libkern/atomic.h>
#include <libkern/lock.h>
#include <libkern/types.h>
#include <mem/kmemzone.h>
#include <mem/memzone.h>
#include <mem/vm_address_space.h>
#include <mem/vmm.h>

#define MAX_PROCESS_COUNT 1024
#define MAX_OPENED_FILES 16

struct blocker;

enum PROC_STATUS {
    PROC_INVALID = 0,
    PROC_ALIVE,
    PROC_DEAD,
    PROC_DYING,
    PROC_ZOMBIE,
};

struct thread;
struct proc {
    vm_address_space_t* address_space;

    pid_t pid;
    pid_t ppid;
    pid_t pgid;
    uint32_t prio;
    uint32_t status;
    struct thread* main_thread;

    spinlock_t lock;

    uid_t uid;
    gid_t gid;
    uid_t euid;
    gid_t egid;
    uid_t suid;
    gid_t sgid;

    file_t* proc_file;
    path_t cwd;
    file_descriptor_t* fds;

    int exit_code;

    bool is_kthread;

    // Trace data
    bool is_tracee;
};
typedef struct proc proc_t;

/**
 * PROC FUNCTIONS
 */
pid_t proc_alloc_pid();
struct thread* thread_by_pid(pid_t pid);

int proc_init_storage();
int proc_setup(proc_t* p);
int proc_setup_with_uid(proc_t* p, uid_t uid, gid_t gid);
int proc_setup_vconsole(proc_t* p, vconsole_entry_t* vconsole);
int proc_fill_up_stack(proc_t* p, int argc, char** argv, char** env);
int proc_free(proc_t* p);
int proc_free_locked(proc_t* p);

struct thread* proc_alloc_thread();
struct thread* proc_create_thread(proc_t* p);
void proc_kill_all_threads(proc_t* p);
void proc_kill_all_threads_except(proc_t* p, struct thread* gthread);

/**
 * KTHREAD FUNCTIONS
 */

int kthread_setup(proc_t* p);
int kthread_setup_regs(proc_t* p, void* entry_point);
void kthread_setup_segment_regs(proc_t* p);
int kthread_fill_up_stack(struct thread* thread, void* data);

int proc_load(proc_t* p, struct thread* main_thread, const char* path);
int proc_fork_from(proc_t* new_proc, struct thread* from_thread);

int proc_die(proc_t* p, int exit_code);
int proc_block_all_threads(proc_t* p, const struct blocker* blocker);

/**
 * PROC FD FUNCTIONS
 */

int proc_chdir(proc_t* p, const char* path);
file_descriptor_t* proc_get_free_fd(proc_t* p);
file_descriptor_t* proc_get_fd(proc_t* p, size_t index);
int proc_get_fd_id(proc_t* proc, file_descriptor_t* fd);
int proc_copy_fd(file_descriptor_t* oldfd, file_descriptor_t* newfd);

/**
 * PROC HELPER FUNCTIONS
 */

static ALWAYS_INLINE bool proc_is_su(proc_t* p) { return p->euid == 0; }
static ALWAYS_INLINE int proc_is_alive(proc_t* p) { return p->status == PROC_ALIVE; }

#endif // _KERNEL_TASKING_PROC_H