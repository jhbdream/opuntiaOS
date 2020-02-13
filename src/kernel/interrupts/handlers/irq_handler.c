#include <irq_handler.h>

void irq_redirect(uint8_t int_no) {
    void (*func)() = handlers[int_no];
    func();
}

void irq_handler(trapframe_t *tf) {
    irq_redirect(tf->int_no);
    if (tf->int_no >= IRQ_SLAVE_OFFSET) {
        port_byte_out(0xA0, 0x20);
    }
    port_byte_out(0x20, 0x20);
}

void irq_empty_handler() {}