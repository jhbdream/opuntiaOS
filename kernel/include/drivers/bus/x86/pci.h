/*
 * Copyright (C) 2020-2022 The opuntiaOS Project Authors.
 *  + Contributed by Nikita Melekhin <nimelehin@gmail.com>
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef _KERNEL_DRIVERS_BUS_X86_PCI_H
#define _KERNEL_DRIVERS_BUS_X86_PCI_H

#include <drivers/driver_manager.h>
#include <libkern/c_attrs.h>
#include <libkern/types.h>
#include <platform/x86/port.h>

struct PACKED pcidd {
    uint8_t bus;
    uint8_t device;
    uint8_t function;

    uint16_t vendor_id;
    uint16_t device_id;

    uint8_t class_id;
    uint8_t subclass_id;
    uint8_t interface_id;
    uint8_t revision_id;

    uint32_t interrupt;
    uint32_t port_base;
};
typedef struct pcidd pcidd_t;

enum bar_type {
    MEMORY_MAPPED = 0,
    INPUT_OUTPUT = 1
};

typedef struct {
    char prefetchable;
    uint32_t address;
    uint8_t type;
} bar_t;

void pci_install();
uint32_t pci_read(uint16_t bus, uint16_t device, uint16_t function, uint32_t offset);
void pci_write(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t data);
char pci_has_device_functions(uint8_t bus, uint8_t device);
int pci_find_devices();
device_desc_t pci_get_device_desriptor(uint8_t bus, uint8_t device, uint8_t function);
uint32_t pci_read_bar(device_t* dev, int bar_id);

#endif /* _KERNEL_DRIVERS_BUS_X86_PCI_H */
