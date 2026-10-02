/* Copyright (c) 2024 Renesas Electronics Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

/* generated pin source file - do not edit */
#include "bsp_api.h"
#include "r_ioport.h"

/* SDRAM pins of the RT-Thread RA8P1 Titan Mini (Winbond W9825G6KH-6, 16-bit
 * bus, 32 MB at 0x68000000). These are configured by R_IOPORT_Open() from
 * R_BSP_WarmStart(BSP_WARM_START_POST_C), before R_BSP_SdramInit().
 * The pin set matches the official Titan Mini FSP pin_data.c exactly (39
 * peripheral-BUS pins; P12_15 is NOT used). Signal comments follow the
 * schematic net names; note the official FSP symbol names label P10_0..P10_4
 * as A3..A0/DQM3 (32-bit-mode naming) - see the board schematic. */
const ioport_pin_cfg_t g_bsp_pin_cfg_data[] = {
    /* Address A0-A4 */
    { .pin = BSP_IO_PORT_10_PIN_04, .pin_cfg = IOPORT_CFG_PERIPHERAL_PIN | IOPORT_PERIPHERAL_BUS }, /* SDRAM A0 */
    { .pin = BSP_IO_PORT_10_PIN_03, .pin_cfg = IOPORT_CFG_PERIPHERAL_PIN | IOPORT_PERIPHERAL_BUS }, /* SDRAM A1 */
    { .pin = BSP_IO_PORT_10_PIN_02, .pin_cfg = IOPORT_CFG_PERIPHERAL_PIN | IOPORT_PERIPHERAL_BUS }, /* SDRAM A2 */
    { .pin = BSP_IO_PORT_10_PIN_01, .pin_cfg = IOPORT_CFG_PERIPHERAL_PIN | IOPORT_PERIPHERAL_BUS }, /* SDRAM A3 */
    { .pin = BSP_IO_PORT_10_PIN_00, .pin_cfg = IOPORT_CFG_PERIPHERAL_PIN | IOPORT_PERIPHERAL_BUS }, /* SDRAM A4 */
    /* Address A5-A12 */
    { .pin = BSP_IO_PORT_05_PIN_03, .pin_cfg = IOPORT_CFG_PERIPHERAL_PIN | IOPORT_PERIPHERAL_BUS }, /* SDRAM A5 */
    { .pin = BSP_IO_PORT_05_PIN_04, .pin_cfg = IOPORT_CFG_PERIPHERAL_PIN | IOPORT_PERIPHERAL_BUS }, /* SDRAM A6 */
    { .pin = BSP_IO_PORT_05_PIN_05, .pin_cfg = IOPORT_CFG_PERIPHERAL_PIN | IOPORT_PERIPHERAL_BUS }, /* SDRAM A7 */
    { .pin = BSP_IO_PORT_05_PIN_06, .pin_cfg = IOPORT_CFG_PERIPHERAL_PIN | IOPORT_PERIPHERAL_BUS }, /* SDRAM A8 */
    { .pin = BSP_IO_PORT_05_PIN_07, .pin_cfg = IOPORT_CFG_PERIPHERAL_PIN | IOPORT_PERIPHERAL_BUS }, /* SDRAM A9 */
    { .pin = BSP_IO_PORT_05_PIN_08, .pin_cfg = IOPORT_CFG_PERIPHERAL_PIN | IOPORT_PERIPHERAL_BUS }, /* SDRAM A10/AP */
    { .pin = BSP_IO_PORT_05_PIN_09, .pin_cfg = IOPORT_CFG_PERIPHERAL_PIN | IOPORT_PERIPHERAL_BUS }, /* SDRAM A11 */
    { .pin = BSP_IO_PORT_05_PIN_10, .pin_cfg = IOPORT_CFG_PERIPHERAL_PIN | IOPORT_PERIPHERAL_BUS }, /* SDRAM A12 */
    /* Bank selects */
    { .pin = BSP_IO_PORT_06_PIN_08, .pin_cfg = IOPORT_CFG_PERIPHERAL_PIN | IOPORT_PERIPHERAL_BUS }, /* SDRAM BA0 */
    { .pin = BSP_IO_PORT_13_PIN_00, .pin_cfg = IOPORT_CFG_PERIPHERAL_PIN | IOPORT_PERIPHERAL_BUS }, /* SDRAM BA1 */
    /* Control and byte-lane masks */
    { .pin = BSP_IO_PORT_08_PIN_13, .pin_cfg = IOPORT_CFG_PERIPHERAL_PIN | IOPORT_PERIPHERAL_BUS }, /* SDRAM CS */
    { .pin = BSP_IO_PORT_10_PIN_06, .pin_cfg = IOPORT_CFG_PERIPHERAL_PIN | IOPORT_PERIPHERAL_BUS }, /* SDRAM CKE */
    { .pin = BSP_IO_PORT_10_PIN_15, .pin_cfg = IOPORT_CFG_PERIPHERAL_PIN | IOPORT_PERIPHERAL_BUS }, /* SDRAM CLK */
    { .pin = BSP_IO_PORT_10_PIN_10, .pin_cfg = IOPORT_CFG_PERIPHERAL_PIN | IOPORT_PERIPHERAL_BUS }, /* SDRAM RAS */
    { .pin = BSP_IO_PORT_10_PIN_09, .pin_cfg = IOPORT_CFG_PERIPHERAL_PIN | IOPORT_CFG_DRIVE_HIGH | IOPORT_PERIPHERAL_BUS }, /* SDRAM CAS */
    { .pin = BSP_IO_PORT_10_PIN_08, .pin_cfg = IOPORT_CFG_PERIPHERAL_PIN | IOPORT_CFG_DRIVE_HIGH | IOPORT_PERIPHERAL_BUS }, /* SDRAM WE */
    { .pin = BSP_IO_PORT_06_PIN_14, .pin_cfg = IOPORT_CFG_PERIPHERAL_PIN | IOPORT_PERIPHERAL_BUS }, /* SDRAM DQM0/LDQM */
    { .pin = BSP_IO_PORT_10_PIN_05, .pin_cfg = IOPORT_CFG_PERIPHERAL_PIN | IOPORT_PERIPHERAL_BUS }, /* SDRAM DQM1/UDQM */
    /* Data DQ0-DQ7 */
    { .pin = BSP_IO_PORT_03_PIN_02, .pin_cfg = IOPORT_CFG_PERIPHERAL_PIN | IOPORT_PERIPHERAL_BUS }, /* SDRAM DQ0 */
    { .pin = BSP_IO_PORT_03_PIN_01, .pin_cfg = IOPORT_CFG_PERIPHERAL_PIN | IOPORT_PERIPHERAL_BUS }, /* SDRAM DQ1 */
    { .pin = BSP_IO_PORT_03_PIN_00, .pin_cfg = IOPORT_CFG_PERIPHERAL_PIN | IOPORT_PERIPHERAL_BUS }, /* SDRAM DQ2 */
    { .pin = BSP_IO_PORT_01_PIN_12, .pin_cfg = IOPORT_CFG_PERIPHERAL_PIN | IOPORT_PERIPHERAL_BUS }, /* SDRAM DQ3 */
    { .pin = BSP_IO_PORT_01_PIN_13, .pin_cfg = IOPORT_CFG_PERIPHERAL_PIN | IOPORT_PERIPHERAL_BUS }, /* SDRAM DQ4 */
    { .pin = BSP_IO_PORT_01_PIN_14, .pin_cfg = IOPORT_CFG_PERIPHERAL_PIN | IOPORT_PERIPHERAL_BUS }, /* SDRAM DQ5 */
    { .pin = BSP_IO_PORT_01_PIN_15, .pin_cfg = IOPORT_CFG_PERIPHERAL_PIN | IOPORT_PERIPHERAL_BUS }, /* SDRAM DQ6 */
    { .pin = BSP_IO_PORT_06_PIN_09, .pin_cfg = IOPORT_CFG_PERIPHERAL_PIN | IOPORT_PERIPHERAL_BUS }, /* SDRAM DQ7 */
    /* Data DQ8-DQ15 */
    { .pin = BSP_IO_PORT_10_PIN_11, .pin_cfg = IOPORT_CFG_PERIPHERAL_PIN | IOPORT_PERIPHERAL_BUS }, /* SDRAM DQ8 */
    { .pin = BSP_IO_PORT_10_PIN_12, .pin_cfg = IOPORT_CFG_PERIPHERAL_PIN | IOPORT_PERIPHERAL_BUS }, /* SDRAM DQ9 */
    { .pin = BSP_IO_PORT_10_PIN_13, .pin_cfg = IOPORT_CFG_PERIPHERAL_PIN | IOPORT_PERIPHERAL_BUS }, /* SDRAM DQ10 */
    { .pin = BSP_IO_PORT_10_PIN_14, .pin_cfg = IOPORT_CFG_PERIPHERAL_PIN | IOPORT_PERIPHERAL_BUS }, /* SDRAM DQ11 */
    { .pin = BSP_IO_PORT_06_PIN_10, .pin_cfg = IOPORT_CFG_PERIPHERAL_PIN | IOPORT_PERIPHERAL_BUS }, /* SDRAM DQ12 */
    { .pin = BSP_IO_PORT_06_PIN_11, .pin_cfg = IOPORT_CFG_PERIPHERAL_PIN | IOPORT_PERIPHERAL_BUS }, /* SDRAM DQ13 */
    { .pin = BSP_IO_PORT_06_PIN_12, .pin_cfg = IOPORT_CFG_PERIPHERAL_PIN | IOPORT_PERIPHERAL_BUS }, /* SDRAM DQ14 */
    { .pin = BSP_IO_PORT_06_PIN_13, .pin_cfg = IOPORT_CFG_PERIPHERAL_PIN | IOPORT_PERIPHERAL_BUS }, /* SDRAM DQ15 */
};

const ioport_cfg_t g_bsp_pin_cfg = {
    .number_of_pins = sizeof(g_bsp_pin_cfg_data) / sizeof(g_bsp_pin_cfg_data[0]),
    .p_pin_cfg_data = g_bsp_pin_cfg_data,
};
