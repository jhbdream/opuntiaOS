/*
 * Copyright (C) 2020-2022 The opuntiaOS Project Authors.
 *  + Contributed by Nikita Melekhin <nimelehin@gmail.com>
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <libkern/kassert.h>
#include <platform/x86/idt.h>
#include <platform/x86/syscalls/params.h>
#include <syscalls/handlers.h>

idt_entry_t idt[IDT_ENTRIES];
irq_handler_t handlers[IDT_ENTRIES];

#define USER true
#define SYS false

static void init_irq_handlers()
{
    for (int i = IRQ_MASTER_OFFSET; i < IRQ_MASTER_OFFSET + 8; i++) {
        handlers[i] = (void*)irq_empty_handler;
    }
    for (int i = IRQ_SLAVE_OFFSET; i < IRQ_SLAVE_OFFSET + 8; i++) {
        handlers[i] = (void*)irq_empty_handler;
    }
}

static void idt_element_setup(uint8_t n, void* handler_ptr, bool is_user)
{
    uintptr_t handler_addr = (uintptr_t)handler_ptr;

    idt[n].offset_lower = handler_addr & 0xffff;
    idt[n].segment = (GDT_SEG_KCODE << 3);
    idt[n].zero = 0;
    idt[n].type = 0x8E;
    if (is_user) {
        idt[n].type |= (0b1100000);
    }
    idt[n].offset_upper = (handler_addr >> 16) & 0xffff;
#ifdef __x86_64__
    idt[n].offset_long = (handler_addr >> 32) & 0xffffffff;
#endif
}

static void lidt(void* ptr, uint16_t size)
{
    uintptr_t p = (uintptr_t)ptr;
#ifdef __i386__
    // Need to use volatile as GCC optimizes out the following writes.
    volatile uint16_t pd[3];
    pd[0] = size - 1;
    pd[1] = p & 0xffff;
    pd[2] = (p >> 16) & 0xffff;
    asm volatile("lidt (%0)"
                 :
                 : "r"(pd));
#elif __x86_64__
    // Need to use volatile as GCC optimizes out the following writes.
    volatile uint16_t pd[5];
    pd[0] = size - 1;
    pd[1] = p & 0xffff;
    pd[2] = (p >> 16) & 0xffff;
    pd[3] = (p >> 32) & 0xffff;
    pd[4] = (p >> 48) & 0xffff;
    asm volatile("lidt (%0)"
                 :
                 : "r"(pd));
#endif
}

void interrupts_setup()
{
    idt_element_setup(0, (void*)isr0, SYS);
    idt_element_setup(1, (void*)isr1, SYS);
    idt_element_setup(2, (void*)isr2, SYS);
    idt_element_setup(3, (void*)isr3, SYS);
    idt_element_setup(4, (void*)isr4, SYS);
    idt_element_setup(5, (void*)isr5, SYS);
    idt_element_setup(6, (void*)isr6, SYS);
    idt_element_setup(7, (void*)isr7, SYS);
    idt_element_setup(8, (void*)isr8, SYS);
    idt_element_setup(9, (void*)isr9, SYS);
    idt_element_setup(10, (void*)isr10, SYS);
    idt_element_setup(11, (void*)isr11, SYS);
    idt_element_setup(12, (void*)isr12, SYS);
    idt_element_setup(13, (void*)isr13, SYS);
    idt_element_setup(14, (void*)isr14, SYS);
    idt_element_setup(15, (void*)isr15, SYS);
    idt_element_setup(16, (void*)isr16, SYS);
    idt_element_setup(17, (void*)isr17, SYS);
    idt_element_setup(18, (void*)isr18, SYS);
    idt_element_setup(19, (void*)isr19, SYS);
    idt_element_setup(20, (void*)isr20, SYS);
    idt_element_setup(21, (void*)isr21, SYS);
    idt_element_setup(22, (void*)isr22, SYS);
    idt_element_setup(23, (void*)isr23, SYS);
    idt_element_setup(24, (void*)isr24, SYS);
    idt_element_setup(25, (void*)isr25, SYS);
    idt_element_setup(26, (void*)isr26, SYS);
    idt_element_setup(27, (void*)isr27, SYS);
    idt_element_setup(28, (void*)isr28, SYS);
    idt_element_setup(29, (void*)isr29, SYS);
    idt_element_setup(30, (void*)isr30, SYS);
    idt_element_setup(31, (void*)isr31, SYS);

    pic_remap(IRQ_MASTER_OFFSET, IRQ_SLAVE_OFFSET);

    idt_element_setup(32, (void*)irq0, SYS);
    idt_element_setup(33, (void*)irq1, SYS);
    idt_element_setup(34, (void*)irq2, SYS);
    idt_element_setup(35, (void*)irq3, SYS);
    idt_element_setup(36, (void*)irq4, SYS);
    idt_element_setup(37, (void*)irq5, SYS);
    idt_element_setup(38, (void*)irq6, SYS);
    idt_element_setup(39, (void*)irq7, SYS);
    idt_element_setup(40, (void*)irq8, SYS);
    idt_element_setup(41, (void*)irq9, SYS);
    idt_element_setup(42, (void*)irq10, SYS);
    idt_element_setup(43, (void*)irq11, SYS);
    idt_element_setup(44, (void*)irq12, SYS);
    idt_element_setup(45, (void*)irq13, SYS);
    idt_element_setup(46, (void*)irq14, SYS);
    idt_element_setup(47, (void*)irq15, SYS);

    for (int i = 48; i < 256; i++) {
        idt_element_setup(i, (void*)syscall, SYS);
    }
    idt_element_setup(SYSCALL_HANDLER_NO, (void*)syscall, USER);

    init_irq_handlers();
    lidt(idt, sizeof(idt));
    system_disable_interrupts_no_counter();
}

void irq_set_dev(irqdev_descritptor_t irqdev_desc)
{
    // This handler is empty for x86.
}

void irq_register_handler(irq_line_t line, irq_priority_t prior, irq_flags_t flags, irq_handler_t func, int cpu_mask)
{
    ASSERT(32 <= line && line < 48);
    handlers[line] = func;
}

irq_line_t irqline_from_id(int id)
{
    return id + IRQ_MASTER_OFFSET;
}
