/*
 * Raspberry Pi emulation (c) 2012 Gregory Estrade
 * Upstreaming code cleanup [including bcm2835_*] (c) 2013 Jan Petrous
 *
 * Rasperry Pi 2 emulation and refactoring Copyright (c) 2015, Microsoft
 * Written by Andrew Baumann
 * 
 * Merge into QEMU root fork John Bradley April 2017
 * 
 * This code is licensed under the GNU GPLv2 and later.
 */

#ifndef BCM2835_CONTROL_H
#define BCM2835_CONTROL_H

#include "hw/sysbus.h"

/* 4 mailboxes per core, for 16 total */
#define BCM2835_NCORES 1
#define BCM2835_MBPERCORE 4

#define TYPE_BCM2835_CONTROL "bcm2836-control"
#define BCM2835_CONTROL(obj) \
    OBJECT_CHECK(BCM2835ControlState, (obj), TYPE_BCM2835_CONTROL)

typedef struct BCM2835ControlState {
    /*< private >*/
    SysBusDevice busdev;
    /*< public >*/
    MemoryRegion iomem;

    /* mailbox state */
    uint32_t mailboxes[BCM2835_NCORES * BCM2835_MBPERCORE];

    /* interrupt routing/control registers */
    uint8_t route_gpu_irq, route_gpu_fiq;
    uint32_t timercontrol[BCM2835_NCORES];
    uint32_t mailboxcontrol[BCM2835_NCORES];

    /* interrupt status regs (derived from input pins; not visible to user) */
    bool gpu_irq, gpu_fiq;
    uint8_t timerirqs[BCM2835_NCORES];

    /* interrupt source registers, post-routing (also input-derived; visible) */
    uint32_t irqsrc[BCM2835_NCORES];
    uint32_t fiqsrc[BCM2835_NCORES];

    /* outputs to CPU cores */
    qemu_irq irq[BCM2835_NCORES];
    qemu_irq fiq[BCM2835_NCORES];
} BCM2835ControlState;

#endif
