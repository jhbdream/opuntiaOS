/*
 * Copyright (C) 2020-2022 The opuntiaOS Project Authors.
 *  + Contributed by Nikita Melekhin <nimelehin@gmail.com>
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <fs/vfs.h>
#include <libkern/bits/errno.h>
#include <libkern/libkern.h>
#include <libkern/log.h>
#include <mem/kmalloc.h>
#include <mem/kmemzone.h>
#include <tasking/elf.h>
#include <tasking/tasking.h>

// #define ELF_DEBUG

static int _elf_load_page_content(memzone_t* zone, uintptr_t vaddr);
static int _elf_swap_page_mode(memzone_t* zone, uintptr_t vaddr);

static vm_ops_t elf_vm_ops = {
    .load_page_content = _elf_load_page_content,
    .restore_swapped_page = NULL,
    .swap_page_mode = _elf_swap_page_mode,
};

static int _elf_load_page_content(memzone_t* zone, uintptr_t vaddr)
{
    void* page_start_ptr = (void*)PAGE_START(vaddr);
    size_t offset_diff = ((uintptr_t)page_start_ptr - zone->vaddr);
    off_t offset = zone->file_offset + offset_diff;
    size_t rem_len = zone->file_size > offset_diff ? zone->file_size - offset_diff : 0;
    rem_len = min(rem_len, VMM_PAGE_SIZE);

    // HACK: See comment below that offset in PH could not be aligned. In this
    // case offset could be negative and should be fixed.
    if (offset < 0) {
        rem_len = rem_len > offset ? rem_len - (-offset) : 0;
        page_start_ptr += -offset;
        offset = 0;
    }

    if (rem_len < VMM_PAGE_SIZE) {
        memset((void*)PAGE_START(vaddr), 0, VMM_PAGE_SIZE);
    }

    spinlock_acquire(&zone->file->lock);
    zone->file->ops->read(zone->file, page_start_ptr, offset, rem_len);
    spinlock_release(&zone->file->lock);
    return 0;
}

static int _elf_swap_page_mode(memzone_t* zone, uintptr_t vaddr)
{
    return SWAP_TO_DEV;
}

static int _elf_load_interpret_program_header_entry(proc_t* p, file_descriptor_t* fd)
{
    memzone_t* zone = NULL;
    elf_program_header_t ph;
    int err = vfs_read(fd, &ph, sizeof(ph));
    if (err != sizeof(ph)) {
        return err;
    }

#ifdef ELF_DEBUG
    log("Got header type %zx %zx - %zx", ph.p_type, ph.p_vaddr, ph.p_memsz);
#endif
    switch (ph.p_type) {
    case PT_LOAD:
        zone = memzone_new(p->address_space, ph.p_vaddr, ph.p_memsz);
        if (!zone) {
            return 0;
        }

        zone->type = ZONE_TYPE_NULL;
        zone->mmu_flags = MMU_FLAG_NONPRIV;
        if (TEST_FLAG(ph.p_flags, PF_X)) {
            zone->mmu_flags |= MMU_FLAG_PERM_EXEC;
        }
        if (TEST_FLAG(ph.p_flags, PF_R)) {
            zone->mmu_flags |= MMU_FLAG_PERM_READ;
        }
        if (TEST_FLAG(ph.p_flags, PF_W)) {
            zone->mmu_flags |= MMU_FLAG_PERM_WRITE;
        }

        // Offset in PH could not be aligned to PAGE_SIZE. Memzones are aligned
        // and when a new zone is added the vaddr and size are aligned to cover
        // pages. So, we have to fix offset and size to match start of zone->vaddr.
        zone->file_offset = ph.p_offset - (ph.p_vaddr - PAGE_START(ph.p_vaddr));
        zone->file_size = ph.p_filesz + (ph.p_vaddr - PAGE_START(ph.p_vaddr));
        zone->file = file_duplicate(fd->file);
        zone->ops = &elf_vm_ops;
        break;
    default:
        break;
    }

    return 0;
}

static int _elf_load_interpret_section_header_entry(proc_t* p, file_descriptor_t* fd)
{
    elf_section_header_t sh;
    int err = vfs_read(fd, &sh, sizeof(sh));
    if (err != sizeof(sh)) {
        return err;
    }

#ifdef ELF_DEBUG
    log("Section type %zx %zx - %zx", sh.sh_type, sh.sh_addr, sh.sh_size);
#endif
    if (!TEST_FLAG(sh.sh_flags, SHF_ALLOC)) {
        // Skip this section as it does not use memory.
        return 0;
    }

    memzone_t* zone = memzone_find(p->address_space, sh.sh_addr);
    if (zone) {
        switch (sh.sh_type) {
        case SHT_PROGBITS:
            zone->type = ZONE_TYPE_CODE;
            break;
        case SHT_NOBITS:
            zone->type = ZONE_TYPE_BSS;
            break;
        case SHT_INIT_ARRAY:
        case SHT_FINI_ARRAY:
        case SHT_PREINIT_ARRAY:
            zone->type = ZONE_TYPE_DATA;
            break;
        default:
            break;
        }
    }

    return 0;
}

static int _elf_load_alloc_stack(proc_t* p)
{
    memzone_t* stack_zone = memzone_new_random_backward(p->address_space, USER_STACK_SIZE);
    stack_zone->type = ZONE_TYPE_STACK;
    stack_zone->mmu_flags = MMU_FLAG_PERM_READ | MMU_FLAG_PERM_WRITE | MMU_FLAG_NONPRIV;
    set_base_pointer(p->main_thread->tf, stack_zone->vaddr + USER_STACK_SIZE);
    set_stack_pointer(p->main_thread->tf, stack_zone->vaddr + USER_STACK_SIZE);
    return 0;
}

static inline int _elf_do_load(proc_t* p, file_descriptor_t* fd, elf_header_t* header)
{
    fd->offset = header->e_phoff;
    int ph_num = header->e_phnum;
    for (int i = 0; i < ph_num; i++) {
        _elf_load_interpret_program_header_entry(p, fd);
    }

    fd->offset = header->e_shoff;
    int sh_num = header->e_shnum;
    for (int i = 0; i < sh_num; i++) {
        _elf_load_interpret_section_header_entry(p, fd);
    }

    if (!memzone_find(p->address_space, 0)) {
        memzone_new(p->address_space, 0, VMM_PAGE_SIZE); // Forbid 0 allocations to handle NULLptrs.
    }
    _elf_load_alloc_stack(p);
    set_instruction_pointer(p->main_thread->tf, header->e_entry);
    return 0;
}

int elf_check_header(elf_header_t* header)
{
    static const char elf_signature[] = { 0x7F, 0x45, 0x4c, 0x46 };
    if (memcmp(header->e_ident, elf_signature, sizeof(elf_signature)) != 0) {
        return -ENOEXEC;
    }

    if (header->e_type != ET_EXEC) {
        return -ENOEXEC;
    }

    if (header->e_machine != MACHINE_ARCH) {
        return -EBADARCH;
    }

    return 0;
}

int elf_load(proc_t* p, file_descriptor_t* fd)
{
    data_access_type_t prev_access_type = THIS_CPU->data_access_type;
    THIS_CPU->data_access_type = DATA_ACCESS_KERNEL;

    elf_header_t header;

    int err = vfs_read(fd, &header, sizeof(header));
    if (err != sizeof(header)) {
        THIS_CPU->data_access_type = prev_access_type;
        return err;
    }

    err = elf_check_header(&header);
    if (err) {
        THIS_CPU->data_access_type = prev_access_type;
        return err;
    }

    err = _elf_do_load(p, fd, &header);
    THIS_CPU->data_access_type = prev_access_type;
    return err;
}

int elf_find_symtab_unchecked(void* mapped_data, void** symtab, size_t* symtab_entries, char** strtab)
{
    *symtab = NULL;
    *symtab_entries = 0;
    *strtab = NULL;
    elf_header_t* header = (elf_header_t*)mapped_data;

    int sh_num = header->e_shnum;
    elf_section_header_t* headers_array = (elf_section_header_t*)(mapped_data + header->e_shoff);
    for (int i = 0; i < sh_num; i++) {
        if (headers_array[i].sh_type == SHT_SYMTAB) {
            *symtab = (void*)(mapped_data + headers_array[i].sh_offset);
            *symtab_entries = headers_array[i].sh_size / sizeof(elf_sym_t);
        }

        if (headers_array[i].sh_type == SHT_STRTAB && header->e_shstrndx != i) {
            *strtab = (void*)(mapped_data + headers_array[i].sh_offset);
        }
    }

    return 0;
}

ssize_t elf_find_function_in_symtab(void* symtab_p, size_t syms_n, uintptr_t ip)
{
    elf_sym_t* symtab = (elf_sym_t*)(symtab_p);
    uint32_t prev = 0;
    ssize_t ans = -1;

    for (size_t i = 0; i < syms_n; i++) {
        if (prev <= symtab[i].st_value && symtab[i].st_value <= ip) {
            prev = symtab[i].st_value;
            ans = symtab[i].st_name;
        }
    }
    return ans;
}