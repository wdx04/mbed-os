/* mbed Microcontroller Library
 * Copyright (c) 2024 ARM Limited
 * SPDX-License-Identifier: Apache-2.0
 */
#include "PeripheralNames.h"
#include "common_data.h"
#include "device.h"

/* The RA8P1 ICU supports 32 external IRQ channels, but the ICU vector table
 * only links a subset of them. mbed supports IRQ0-IRQ15 and IRQ29 (used by
 * the on-board user button); entries 16-28 stay NULL and no pin in
 * PinMap_IRQ may map to them. */
icu_instance_ctrl_t * const g_icu_ctrl[IRQ_CHANNELS_COUNT] = {
    &g_external_irq0_ctrl,
    &g_external_irq1_ctrl,
    &g_external_irq2_ctrl,
    &g_external_irq3_ctrl,
    &g_external_irq4_ctrl,
    &g_external_irq5_ctrl,
    &g_external_irq6_ctrl,
    &g_external_irq7_ctrl,
    &g_external_irq8_ctrl,
    &g_external_irq9_ctrl,
    &g_external_irq10_ctrl,
    &g_external_irq11_ctrl,
    &g_external_irq12_ctrl,
    &g_external_irq13_ctrl,
    &g_external_irq14_ctrl,
    &g_external_irq15_ctrl,
    [16 ... 28] = NULL,
    &g_external_irq29_ctrl,
};

const external_irq_cfg_t * const g_icu_cfg[IRQ_CHANNELS_COUNT] = {
    &g_external_irq0_cfg,
    &g_external_irq1_cfg,
    &g_external_irq2_cfg,
    &g_external_irq3_cfg,
    &g_external_irq4_cfg,
    &g_external_irq5_cfg,
    &g_external_irq6_cfg,
    &g_external_irq7_cfg,
    &g_external_irq8_cfg,
    &g_external_irq9_cfg,
    &g_external_irq10_cfg,
    &g_external_irq11_cfg,
    &g_external_irq12_cfg,
    &g_external_irq13_cfg,
    &g_external_irq14_cfg,
    &g_external_irq15_cfg,
    [16 ... 28] = NULL,
    &g_external_irq29_cfg,
};
