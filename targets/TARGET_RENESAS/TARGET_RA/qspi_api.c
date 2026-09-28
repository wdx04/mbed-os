/* mbed Microcontroller Library
 * Copyright (c) 2026 ARM Limited
 * SPDX-License-Identifier: Apache-2.0
 *
 * QSPI HAL implementation for Renesas RA targets.
 *
 * Two implementations share this file:
 *  - TARGET_RA8P1: the OSPI-B (XSPI) controller driving a generic quad SPI
 *    flash through FSP r_ospi_b. See the comment block below.
 *  - everything else: the legacy RA QSPI peripheral (SFM* registers) driven
 *    with a bit-beat sequencer. See the comment block of that section.
 */

#if DEVICE_QSPI

#include "qspi_api.h"
#include "mbed_error.h"
#include "pinmap.h"
#include "PeripheralPins.h"
#include "objects.h"
#include "bsp_api.h"

#if defined(TARGET_RA8P1)

/* ==========================================================================
 * RA8P1: OSPI-B (XSPI) implementation
 * ==========================================================================
 *
 * The RA8P1 has no legacy QSPI peripheral; quad SPI flashes sit behind the
 * OSPI-B (XSPI) controller, driven here through the FSP r_ospi_b driver on
 * unit 0 / device select 0 (XIP window at 0x80000000).
 *
 * The XSPI controller cannot sequence arbitrary per-phase bus-width frames,
 * but the generic mbed QSPI HAL traffic (QSPIFBlockDevice) falls into three
 * frame shapes that map directly onto XSPI protocol modes:
 *
 *   - all control commands (WREN/WRDI, read/write status, RDID, resets,
 *     EN4B, erases) are 1S-1S-1S and small (<= 8 data bytes): executed with
 *     R_OSPI_B_DirectTransfer, which supports a per-call command/address/
 *     data/dummy specification inside the currently selected protocol.
 *     Longer or addressed reads in 3-byte addressing (the SFDP probing
 *     phase, before EN4B) are chunked through DirectTransfer 8 bytes at a
 *     time.
 *   - data reads are issued in the best mode QSPIFBlockDevice negotiated
 *     from SFDP (1S-1S-4S / 1S-4S-4S, 4-byte addressing): served by
 *     switching the protocol (which reloads the memory-mapped read command
 *     and dummy cycles from our command set entry) and memcpy-ing from the
 *     XIP window.
 *   - page programs (1S-1S-1S, 4-byte addressing) go through
 *     R_OSPI_B_Write, which issues the entry's program command with write
 *     enable via the DMAC and blocks until completion. Longer writes are
 *     split at page boundaries.
 *
 * An XSPI mode/alt phase cannot be sequenced; alt (mode) bits requested by
 * the upper layer are folded into the XIP read's dummy cycles (alt size in
 * bits / data lines).
 *
 * Only SPI mode 0 is supported. The bus frequency is fixed by the BSP
 * OCTACLK configuration (see the board's bsp_clock_cfg.h) and the frequency
 * argument is only recorded.
 */

#include <string.h>
#include "r_ospi_b.h"
#include "qspi_xip.h"

/* DMAC channel 2 is reserved for the OSPI peripheral by the board support
 * (hal_data.c, g_transfer2). r_ospi_b requires a transfer instance when
 * built with OSPI_B_CFG_DMAC_SUPPORT_ENABLE. */
extern const transfer_instance_t g_transfer2;

#define QSPI_OSPI_XIP_BASE      0x80000000UL
#define QSPI_OSPI_PAGE_SIZE     256U
#define QSPI_OSPI_DIRECT_MAX    8U      /* DirectTransfer data buffer size */

static ospi_b_instance_ctrl_t qspi_ospi_ctrl;
static bool qspi_ospi_open;
static uint32_t qspi_ospi_users;    /* active qspi_t objects sharing the controller */

static const ospi_b_timing_setting_t qspi_ospi_timing =
{
    .command_to_command_interval = OSPI_B_COMMAND_INTERVAL_CLOCKS_2,
    .cs_pullup_lag               = OSPI_B_COMMAND_CS_PULLUP_CLOCKS_NO_EXTENSION,
    .cs_pulldown_lead            = OSPI_B_COMMAND_CS_PULLDOWN_CLOCKS_NO_EXTENSION,
    .sdr_drive_timing            = OSPI_B_SDR_DRIVE_TIMING_BEFORE_CK,
    .sdr_sampling_edge           = OSPI_B_CK_EDGE_FALLING,
    .sdr_sampling_delay          = OSPI_B_SDR_SAMPLING_DELAY_NONE,
    .ddr_sampling_extension      = OSPI_B_DDR_SAMPLING_EXTENSION_NONE,
};

static const spi_flash_erase_command_t qspi_ospi_erase_commands[] =
{
    { .command = 0x21, .size = 4096 },
    { .command = 0xDC, .size = 65536 },
    { .command = 0x60, .size = SPI_FLASH_ERASE_SIZE_CHIP_ERASE },
};

static const ospi_b_table_t qspi_ospi_erase_table =
{
    .p_table = (void *)qspi_ospi_erase_commands,
    .length  = sizeof(qspi_ospi_erase_commands) / sizeof(qspi_ospi_erase_commands[0]),
};

/* One entry per protocol mode we switch between. The read command/dummy
 * cycles and the program command are rewritten before each memory-mapped
 * operation to whatever the upper layer selected, so the table is mutable
 * and R_OSPI_B_SpiProtocolSet re-caches the entry every time. Addressing
 * is fixed to 4 bytes: the XIP fast paths only run after QSPIFBlockDevice
 * has switched the flash to 4-byte addressing (EN4B). */
static ospi_b_xspi_command_set_t qspi_ospi_cmd_sets[] =
{
    {
        /* 1S-1S-1S: control commands, slow read (0x13), page program (0x12) */
        .protocol = SPI_FLASH_PROTOCOL_EXTENDED_SPI,
        .frame_format = OSPI_B_FRAME_FORMAT_STANDARD,
        .latency_mode = OSPI_B_LATENCY_MODE_FIXED,
        .command_bytes = OSPI_B_COMMAND_BYTES_1,
        .address_bytes = SPI_FLASH_ADDRESS_BYTES_4,
        .read_command = 0x13,
        .program_command = 0x12,
        .write_enable_command = 0x06,
        .status_command = 0x05,
        .row_load_command = 0x0,
        .row_store_command = 0x0,
        .read_dummy_cycles = 0,
        .program_dummy_cycles = 0,
        .status_dummy_cycles = 0,
        .row_load_dummy_cycles = 0,
        .row_store_dummy_cycles = 0,
        .address_msb_mask = 0xF0,
        .status_needs_address = false,
        .status_address = 0U,
        .status_address_bytes = (spi_flash_address_bytes_t)0U,
        .p_erase_commands = &qspi_ospi_erase_table,
    },
    {
        /* 1S-1S-4S: fast read quad output (0x6C), quad page program (0x34) */
        .protocol = SPI_FLASH_PROTOCOL_1S_1S_4S,
        .frame_format = OSPI_B_FRAME_FORMAT_STANDARD,
        .latency_mode = OSPI_B_LATENCY_MODE_FIXED,
        .command_bytes = OSPI_B_COMMAND_BYTES_1,
        .address_bytes = SPI_FLASH_ADDRESS_BYTES_4,
        .read_command = 0x6C,
        .program_command = 0x34,
        .write_enable_command = 0x06,
        .status_command = 0x05,
        .row_load_command = 0x0,
        .row_store_command = 0x0,
        .read_dummy_cycles = 8,
        .program_dummy_cycles = 0,
        .status_dummy_cycles = 0,
        .row_load_dummy_cycles = 0,
        .row_store_dummy_cycles = 0,
        .address_msb_mask = 0xF0,
        .status_needs_address = false,
        .status_address = 0U,
        .status_address_bytes = (spi_flash_address_bytes_t)0U,
        .p_erase_commands = &qspi_ospi_erase_table,
    },
    {
        /* 1S-4S-4S: fast read quad I/O (0xEC), quad page program (0x34) */
        .protocol = SPI_FLASH_PROTOCOL_1S_4S_4S,
        .frame_format = OSPI_B_FRAME_FORMAT_STANDARD,
        .latency_mode = OSPI_B_LATENCY_MODE_FIXED,
        .command_bytes = OSPI_B_COMMAND_BYTES_1,
        .address_bytes = SPI_FLASH_ADDRESS_BYTES_4,
        .read_command = 0xEC,
        .program_command = 0x34,
        .write_enable_command = 0x06,
        .status_command = 0x05,
        .row_load_command = 0x0,
        .row_store_command = 0x0,
        .read_dummy_cycles = 6,
        .program_dummy_cycles = 0,
        .status_dummy_cycles = 0,
        .row_load_dummy_cycles = 0,
        .row_store_dummy_cycles = 0,
        .address_msb_mask = 0xF0,
        .status_needs_address = false,
        .status_address = 0U,
        .status_address_bytes = (spi_flash_address_bytes_t)0U,
        .p_erase_commands = &qspi_ospi_erase_table,
    },
    {
        /* 1S-1S-1S: fast read quad I/O window reads in plain SPI mode
         * (PRTMD=1). The EXTENDED_SPI entry above (PRTMD=0) is used for
         * direct commands only - window reads under it come back shifted. */
        .protocol = SPI_FLASH_PROTOCOL_1S_1S_1S,
        .frame_format = OSPI_B_FRAME_FORMAT_STANDARD,
        .latency_mode = OSPI_B_LATENCY_MODE_FIXED,
        .command_bytes = OSPI_B_COMMAND_BYTES_1,
        .address_bytes = SPI_FLASH_ADDRESS_BYTES_4,
        .read_command = 0x13,
        .program_command = 0x12,
        .write_enable_command = 0x06,
        .status_command = 0x05,
        .row_load_command = 0x0,
        .row_store_command = 0x0,
        .read_dummy_cycles = 0,
        .program_dummy_cycles = 0,
        .status_dummy_cycles = 0,
        .row_load_dummy_cycles = 0,
        .row_store_dummy_cycles = 0,
        .address_msb_mask = 0xF0,
        .status_needs_address = false,
        .status_address = 0U,
        .status_address_bytes = (spi_flash_address_bytes_t)0U,
        .p_erase_commands = &qspi_ospi_erase_table,
    },
    {
        /* 1S-2S-2S: fast read dual I/O (0xBC, 4-byte address) */
        .protocol = SPI_FLASH_PROTOCOL_1S_2S_2S,
        .frame_format = OSPI_B_FRAME_FORMAT_STANDARD,
        .latency_mode = OSPI_B_LATENCY_MODE_FIXED,
        .command_bytes = OSPI_B_COMMAND_BYTES_1,
        .address_bytes = SPI_FLASH_ADDRESS_BYTES_4,
        .read_command = 0xBC,
        .program_command = 0x02,
        .write_enable_command = 0x06,
        .status_command = 0x05,
        .row_load_command = 0x0,
        .row_store_command = 0x0,
        .read_dummy_cycles = 4,
        .program_dummy_cycles = 0,
        .status_dummy_cycles = 0,
        .row_load_dummy_cycles = 0,
        .row_store_dummy_cycles = 0,
        .address_msb_mask = 0xF0,
        .status_needs_address = false,
        .status_address = 0U,
        .status_address_bytes = (spi_flash_address_bytes_t)0U,
        .p_erase_commands = &qspi_ospi_erase_table,
    },
};

static const ospi_b_table_t qspi_ospi_cmd_set_table =
{
    .p_table = (void *)qspi_ospi_cmd_sets,
    .length  = sizeof(qspi_ospi_cmd_sets) / sizeof(qspi_ospi_cmd_sets[0]),
};

static const ospi_b_extended_cfg_t qspi_ospi_extend_cfg =
{
    .ospi_b_unit                             = 0,
    .channel                                 = (ospi_b_device_number_t)0,
    .p_timing_settings                       = &qspi_ospi_timing,
    .p_xspi_command_set                      = &qspi_ospi_cmd_set_table,
    .data_latch_delay_clocks                 = OSPI_B_DS_TIMING_DELAY_NONE,
    .p_autocalibration_preamble_pattern_addr = (uint8_t *)0,
#if OSPI_B_CFG_DMAC_SUPPORT_ENABLE
    .p_lower_lvl_transfer                    = &g_transfer2,
#endif
};

static const spi_flash_cfg_t qspi_ospi_cfg =
{
    .spi_protocol               = SPI_FLASH_PROTOCOL_EXTENDED_SPI,
    .read_mode                  = SPI_FLASH_READ_MODE_STANDARD,
    .address_bytes              = SPI_FLASH_ADDRESS_BYTES_4,
    .dummy_clocks               = SPI_FLASH_DUMMY_CLOCKS_DEFAULT,
    .page_program_address_lines = (spi_flash_data_lines_t)0U,
    .write_status_bit           = 0,
    .write_enable_bit           = 1,
    .page_size_bytes            = QSPI_OSPI_PAGE_SIZE,
    .page_program_command       = 0,
    .write_enable_command       = 0,
    .status_command             = 0,
    .read_command               = 0,
    .xip_enter_command          = 0,
    .xip_exit_command           = 0,
    .erase_command_list_length  = sizeof(qspi_ospi_erase_commands) / sizeof(qspi_ospi_erase_commands[0]),
    .p_erase_command_list       = qspi_ospi_erase_commands,
    .p_extend                   = &qspi_ospi_extend_cfg,
};

/* Map the per-phase bus widths of a frame onto an XSPI protocol mode.
 * QSPIFBlockDevice only issues these three combinations (quad page program
 * with a quad address phase does not exist on SPI NOR flashes). */
static qspi_status_t qspi_ospi_map_protocol(const qspi_command_t *command, spi_flash_protocol_t *protocol)
{
    if (!command->instruction.disabled &&
            command->instruction.bus_width != QSPI_CFG_BUS_SINGLE) {
        return QSPI_STATUS_INVALID_PARAMETER;
    }

    bool addr_quad = !command->address.disabled &&
                     command->address.bus_width == QSPI_CFG_BUS_QUAD;
    bool data_quad = command->data.bus_width == QSPI_CFG_BUS_QUAD;

    if (addr_quad && data_quad) {
        *protocol = SPI_FLASH_PROTOCOL_1S_4S_4S;
    } else if (!addr_quad && data_quad) {
        *protocol = SPI_FLASH_PROTOCOL_1S_1S_4S;
    } else if (!addr_quad && !data_quad) {
        *protocol = SPI_FLASH_PROTOCOL_EXTENDED_SPI;
    } else {
        /* quad address with single data: not used by SPI NOR */
        return QSPI_STATUS_INVALID_PARAMETER;
    }

    return QSPI_STATUS_OK;
}

static qspi_status_t qspi_ospi_set_protocol(spi_flash_protocol_t protocol)
{
    if (FSP_SUCCESS != R_OSPI_B_SpiProtocolSet(&qspi_ospi_ctrl, protocol)) {
        return QSPI_STATUS_ERROR;
    }
    return QSPI_STATUS_OK;
}

/* Command set table entry for a protocol mode (entries are matched by the
 * protocol field). NULL if the table has no entry for it. */
static ospi_b_xspi_command_set_t *qspi_ospi_entry_for(spi_flash_protocol_t protocol)
{
    for (uint32_t i = 0; i < qspi_ospi_cmd_set_table.length; i++) {
        if (qspi_ospi_cmd_sets[i].protocol == protocol) {
            return &qspi_ospi_cmd_sets[i];
        }
    }
    return NULL;
}

/* XIP reads go through the CPU cache; drop lines covering flash content
 * that changed behind the cache (program/erase). */
static void qspi_ospi_invalidate(uint32_t addr, uint32_t length)
{
    SCB_InvalidateDCache_by_Addr((void *)(QSPI_OSPI_XIP_BASE + addr), (int32_t)length);
}

/* qspi_address_size_t is an enum (index), not a bit count. */
static uint8_t qspi_ospi_addr_bytes(qspi_address_size_t size)
{
    switch (size) {
        case QSPI_CFG_ADDR_SIZE_8:
            return 1;
        case QSPI_CFG_ADDR_SIZE_16:
            return 2;
        case QSPI_CFG_ADDR_SIZE_24:
            return 3;
        default:
            return 4;
    }
}

/* Issue one manual-command transaction. `data` holds up to 8 bytes,
 * little-endian: the first byte sent/received occupies the least
 * significant byte of CDD0 (verified with RDID on the W25Q256JVEQ). */
static qspi_status_t qspi_ospi_direct(uint32_t command, uint32_t address, uint8_t address_length,
                                      uint64_t *data, uint8_t data_length, uint8_t dummy_cycles,
                                      spi_flash_direct_transfer_dir_t dir)
{
    spi_flash_direct_transfer_t xfer;
    memset(&xfer, 0, sizeof(xfer));
    xfer.command        = command;
    xfer.command_length = 1;
    xfer.address        = address;
    xfer.address_length = address_length;
    xfer.data_length    = data_length;
    xfer.dummy_cycles   = dummy_cycles;
    if ((NULL != data) && (SPI_FLASH_DIRECT_TRANSFER_DIR_WRITE == dir)) {
        xfer.data_u64 = *data;
    }

    if (FSP_SUCCESS != R_OSPI_B_DirectTransfer(&qspi_ospi_ctrl, &xfer, dir)) {
        return QSPI_STATUS_ERROR;
    }

    if ((NULL != data) && (SPI_FLASH_DIRECT_TRANSFER_DIR_READ == dir)) {
        *data = xfer.data_u64;
    }
    return QSPI_STATUS_OK;
}

static uint64_t qspi_ospi_pack(const uint8_t *src, size_t length)
{
    uint64_t value = 0;
    for (size_t i = 0; i < length; i++) {
        value |= ((uint64_t)src[i]) << (8 * i);
    }
    return value;
}

static void qspi_ospi_unpack(uint64_t value, uint8_t *dst, size_t length)
{
    for (size_t i = 0; i < length; i++) {
        dst[i] = (uint8_t)(value >> (8 * i));
    }
}

/* Fold a mode/alt phase into dummy cycles for the XIP read path. */
static uint8_t qspi_ospi_total_dummy(const qspi_command_t *command)
{
    uint8_t dummy = command->dummy_count;
    if (!command->alt.disabled && command->alt.size > 0) {
        int lines = (command->data.bus_width == QSPI_CFG_BUS_QUAD) ? 4 :
                    (command->data.bus_width == QSPI_CFG_BUS_DUAL) ? 2 : 1;
        dummy = (uint8_t)(dummy + command->alt.size / lines);
    }
    return dummy;
}

#if STATIC_PINMAP_READY
#define QSPI_INIT_DIRECT qspi_init_direct
qspi_status_t qspi_init_direct(qspi_t *obj, const qspi_pinmap_t *pinmap, uint32_t hz, uint8_t mode)
#else
#define QSPI_INIT_DIRECT _qspi_init_direct
static qspi_status_t _qspi_init_direct(qspi_t *obj, const qspi_pinmap_t *pinmap, uint32_t hz, uint8_t mode)
#endif
{
    /* The OSPI controller only supports SPI mode 0. */
    if (mode != 0) {
        return QSPI_STATUS_INVALID_PARAMETER;
    }

    /* Configure the OSPI-B pins (no pin_mode() here - on RA each pin_function
     * call rewrites the whole PFS register). */
    obj->io0 = pinmap->data0_pin;
    pin_function(pinmap->data0_pin, pinmap->data0_function);
    obj->io1 = pinmap->data1_pin;
    pin_function(pinmap->data1_pin, pinmap->data1_function);
    obj->io2 = pinmap->data2_pin;
    pin_function(pinmap->data2_pin, pinmap->data2_function);
    obj->io3 = pinmap->data3_pin;
    pin_function(pinmap->data3_pin, pinmap->data3_function);
    obj->sclk = pinmap->sclk_pin;
    pin_function(pinmap->sclk_pin, pinmap->sclk_function);
    obj->ssel = pinmap->ssel_pin;
    pin_function(pinmap->ssel_pin, pinmap->ssel_function);

    obj->hz = (int)hz;
    obj->xip_active = false;
    obj->xip_exit_command = 0U;

    /* The OSPI controller is shared by every qspi_t object (single flash
     * device): open it with the first user only. */
    qspi_ospi_users++;
    if (!qspi_ospi_open) {
        fsp_err_t err = R_OSPI_B_Open(&qspi_ospi_ctrl, &qspi_ospi_cfg);
        if (FSP_SUCCESS != err && FSP_ERR_ALREADY_OPEN != err) {
            qspi_ospi_users--;
            return QSPI_STATUS_ERROR;
        }
        qspi_ospi_open = true;
        R_OSPI_B_SpiProtocolSet(&qspi_ospi_ctrl, SPI_FLASH_PROTOCOL_EXTENDED_SPI);

        /* Deterministic flash state: soft reset returns the device to its
         * power-up state (3-byte addressing, QE cleared is NOT among them -
         * QE is non-volatile) in case a previous boot left it in 4-byte
         * addressing. QSPIFBlockDevice runs its own detection afterwards. */
        qspi_ospi_direct(0x66, 0, 0, NULL, 0, 0, SPI_FLASH_DIRECT_TRANSFER_DIR_WRITE);
        R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MICROSECONDS);
        qspi_ospi_direct(0x99, 0, 0, NULL, 0, 0, SPI_FLASH_DIRECT_TRANSFER_DIR_WRITE);
        R_BSP_SoftwareDelay(50, BSP_DELAY_UNITS_MICROSECONDS);
    }

    return QSPI_STATUS_OK;
}

qspi_status_t qspi_free(qspi_t *obj)
{
    /* Only the last user tears the controller down; qspi_t objects come and
     * go (the mbed QSPI driver class opens one per instance) but they all
     * share one flash device. */
    if (qspi_ospi_users > 0) {
        qspi_ospi_users--;
    }

    if ((0 == qspi_ospi_users) && qspi_ospi_open) {
        R_OSPI_B_Close(&qspi_ospi_ctrl);
        qspi_ospi_open = false;

        /* Release the pins (function 0 = analog/reset state). */
        pin_function(obj->io0, 0);
        pin_function(obj->io1, 0);
        pin_function(obj->io2, 0);
        pin_function(obj->io3, 0);
        pin_function(obj->sclk, 0);
        pin_function(obj->ssel, 0);
    }

    return QSPI_STATUS_OK;
}

qspi_status_t qspi_frequency(qspi_t *obj, int hz)
{
    if (hz <= 0) {
        return QSPI_STATUS_INVALID_PARAMETER;
    }

    /* The OSPI clock is fixed by the BSP OCTACLK configuration; only record
     * the request. */
    obj->hz = hz;
    return QSPI_STATUS_OK;
}

qspi_status_t qspi_write(qspi_t *obj, const qspi_command_t *command, const void *data, size_t *length)
{
    (void)obj;

    if ((data == NULL) || (length == NULL) || (*length == 0)) {
        return QSPI_STATUS_INVALID_PARAMETER;
    }

    spi_flash_protocol_t protocol;
    qspi_status_t status = qspi_ospi_map_protocol(command, &protocol);
    if (status != QSPI_STATUS_OK) {
        return status;
    }

    uint32_t addr = command->address.value;
    uint32_t written = 0;

    if ((command->address.disabled) ||
            (command->address.size != QSPI_CFG_ADDR_SIZE_32) ||
            (command->instruction.disabled) ||
            (command->data.bus_width != QSPI_CFG_BUS_SINGLE)) {
        /* SFDP-era or narrow writes: chunked manual commands (8 bytes each).
         * The write-enable latch is managed by the upper layer. */
        if (*length > QSPI_OSPI_DIRECT_MAX) {
            return QSPI_STATUS_INVALID_PARAMETER;
        }
        status = qspi_ospi_set_protocol(SPI_FLASH_PROTOCOL_EXTENDED_SPI);
        if (status != QSPI_STATUS_OK) {
            return status;
        }
        uint8_t addr_len = command->address.disabled ? 0 :
                           qspi_ospi_addr_bytes(command->address.size);
        uint64_t value = qspi_ospi_pack((const uint8_t *)data, *length);
        return qspi_ospi_direct(command->instruction.value, addr, addr_len, &value,
                                (uint8_t)*length, command->dummy_count,
                                SPI_FLASH_DIRECT_TRANSFER_DIR_WRITE);
    }

    /* Page program through the XIP window (1S-1S-1S, 4-byte addressing):
     * R_OSPI_B_Write issues the entry's program command with write enable
     * and moves the data with the DMAC. Split at page boundaries. */
    qspi_ospi_cmd_sets[0].program_command = command->instruction.value;
    status = qspi_ospi_set_protocol(SPI_FLASH_PROTOCOL_EXTENDED_SPI);
    if (status != QSPI_STATUS_OK) {
        return status;
    }

    while (written < *length) {
        uint32_t chunk = QSPI_OSPI_PAGE_SIZE - ((addr + written) % QSPI_OSPI_PAGE_SIZE);
        if (chunk > (*length - written)) {
            chunk = (uint32_t)(*length - written);
        }
        if (FSP_SUCCESS != R_OSPI_B_Write(&qspi_ospi_ctrl,
                                          (const uint8_t *)data + written,
                                          (uint8_t *)(QSPI_OSPI_XIP_BASE + addr + written),
                                          chunk)) {
            return QSPI_STATUS_ERROR;
        }
        written += chunk;
    }

    qspi_ospi_invalidate(addr, (uint32_t)*length);
    return QSPI_STATUS_OK;
}

qspi_status_t qspi_read(qspi_t *obj, const qspi_command_t *command, void *data, size_t *length)
{
    (void)obj;

    if ((data == NULL) || (length == NULL) || (*length == 0)) {
        return QSPI_STATUS_INVALID_PARAMETER;
    }

    spi_flash_protocol_t protocol;
    qspi_status_t status = qspi_ospi_map_protocol(command, &protocol);
    if (status != QSPI_STATUS_OK) {
        return status;
    }

    uint32_t addr = command->address.value;

    if ((command->address.disabled) ||
            (command->address.size != QSPI_CFG_ADDR_SIZE_32) ||
            (command->instruction.disabled)) {
        /* SFDP probing phase (0x5A) and other addressed reads before 4-byte
         * addressing: chunked manual commands, 8 bytes per transaction. */
        status = qspi_ospi_set_protocol(SPI_FLASH_PROTOCOL_EXTENDED_SPI);
        if (status != QSPI_STATUS_OK) {
            return status;
        }
        uint8_t addr_len = command->address.disabled ? 0 :
                           qspi_ospi_addr_bytes(command->address.size);
        size_t done = 0;
        while (done < *length) {
            size_t chunk = *length - done;
            if (chunk > QSPI_OSPI_DIRECT_MAX) {
                chunk = QSPI_OSPI_DIRECT_MAX;
            }
            uint64_t value = 0;
            status = qspi_ospi_direct(command->instruction.value,
                                      command->address.disabled ? 0 : addr + done, addr_len,
                                      &value, (uint8_t)chunk, command->dummy_count,
                                      SPI_FLASH_DIRECT_TRANSFER_DIR_READ);
            if (status != QSPI_STATUS_OK) {
                return status;
            }
            qspi_ospi_unpack(value, (uint8_t *)data + done, chunk);
            done += chunk;
        }
        return QSPI_STATUS_OK;
    }

    /* Fast path: memory-mapped read in the selected protocol. Rewrite the
     * entry's read command and dummy cycles so the XIP transaction matches
     * the frame the upper layer asked for, then memcpy from the window. */
    ospi_b_xspi_command_set_t *entry = qspi_ospi_entry_for(protocol);
    if (NULL == entry) {
        return QSPI_STATUS_INVALID_PARAMETER;
    }
    entry->read_command       = command->instruction.value;
    entry->read_dummy_cycles  = qspi_ospi_total_dummy(command);
    entry->address_bytes      = SPI_FLASH_ADDRESS_BYTES_4;

    status = qspi_ospi_set_protocol(protocol);
    if (status != QSPI_STATUS_OK) {
        return status;
    }

    memcpy(data, (const void *)(QSPI_OSPI_XIP_BASE + addr), *length);

    return QSPI_STATUS_OK;
}

qspi_status_t qspi_command_transfer(qspi_t *obj, const qspi_command_t *command,
                                    const void *tx_data, size_t tx_size,
                                    void *rx_data, size_t rx_size)
{
    (void)obj;

    if (command->instruction.disabled) {
        return QSPI_STATUS_INVALID_PARAMETER;
    }
    if ((tx_size > QSPI_OSPI_DIRECT_MAX) || (rx_size > QSPI_OSPI_DIRECT_MAX)) {
        return QSPI_STATUS_INVALID_PARAMETER;
    }

    /* Control commands are always 1S-1S-1S on SPI NOR. */
    if ((command->instruction.bus_width != QSPI_CFG_BUS_SINGLE) ||
            (!command->address.disabled &&
             command->address.bus_width != QSPI_CFG_BUS_SINGLE)) {
        return QSPI_STATUS_INVALID_PARAMETER;
    }

    spi_flash_protocol_t protocol;
    qspi_status_t status = qspi_ospi_map_protocol(command, &protocol);
    if (status != QSPI_STATUS_OK) {
        return status;
    }

    /* Frame the transaction explicitly: a previous memory-mapped read may
     * have left the controller in a quad protocol. */
    status = qspi_ospi_set_protocol(protocol);
    if (status != QSPI_STATUS_OK) {
        return status;
    }

    uint8_t addr_len = command->address.disabled ? 0 :
                       qspi_ospi_addr_bytes(command->address.size);
    uint32_t addr = command->address.disabled ? 0 : command->address.value;

    /* Transmit first, then receive with the same command (two bus cycles),
     * matching the other implementations. Command-only frames (WREN, WRDI,
     * EN4B, reset sequences...) carry no data and must still be clocked
     * out. */
    if ((tx_data != NULL) && (tx_size > 0)) {
        uint64_t value = qspi_ospi_pack((const uint8_t *)tx_data, tx_size);
        status = qspi_ospi_direct(command->instruction.value, addr, addr_len, &value,
                                  (uint8_t)tx_size, command->dummy_count,
                                  SPI_FLASH_DIRECT_TRANSFER_DIR_WRITE);
        if (status != QSPI_STATUS_OK) {
            return status;
        }
    } else if ((rx_data != NULL) && (rx_size > 0)) {
        uint64_t value = 0;
        status = qspi_ospi_direct(command->instruction.value, addr, addr_len, &value,
                                  (uint8_t)rx_size, command->dummy_count,
                                  SPI_FLASH_DIRECT_TRANSFER_DIR_READ);
        if (status != QSPI_STATUS_OK) {
            return status;
        }
        qspi_ospi_unpack(value, (uint8_t *)rx_data, rx_size);
    } else {
        /* Command-only transaction. */
        status = qspi_ospi_direct(command->instruction.value, addr, addr_len, NULL, 0,
                                  command->dummy_count,
                                  SPI_FLASH_DIRECT_TRANSFER_DIR_WRITE);
        if (status != QSPI_STATUS_OK) {
            return status;
        }
    }

    /* A write-direction command that carries an address is an erase (or
     * similar content-changing operation): the CPU cache may hold stale
     * lines for the XIP window, drop them all. */
    if (!command->address.disabled && (rx_data == NULL || rx_size == 0)) {
        qspi_ospi_invalidate(0, 32U * 1024U * 1024U);
    }

    return QSPI_STATUS_OK;
}

qspi_status_t qspi_xip_enter(qspi_t *obj, const qspi_xip_config_t *config)
{
    if ((obj == NULL) || (config == NULL)) {
        return QSPI_STATUS_INVALID_PARAMETER;
    }
    if (config->read_mode > QSPI_XIP_READ_QUAD_IO) {
        return QSPI_STATUS_INVALID_PARAMETER;
    }
    if ((config->address_bytes < 1U) || (config->address_bytes > 4U)) {
        return QSPI_STATUS_INVALID_PARAMETER;
    }
    if ((config->dummy_clocks != QSPI_XIP_DUMMY_DEFAULT) &&
            (config->dummy_clocks > 63U)) {
        /* The XSPI latency fields are 6 bits wide. */
        return QSPI_STATUS_INVALID_PARAMETER;
    }
    if (0U != config->xip_enter_command) {
        /* Flash continuous-read ("true XIP") needs a mode/alternate byte
         * phase, which the XSPI standard SPI frame format cannot emit.
         * Only the memory-mapped window is supported - see qspi_xip.h. */
        return QSPI_STATUS_INVALID_PARAMETER;
    }
    if (obj->xip_active) {
        return QSPI_STATUS_ERROR;
    }

    static const struct {
        spi_flash_protocol_t protocol;
        uint8_t default_command;
        uint8_t default_dummy;
    } modes[] = {
        [QSPI_XIP_READ_STANDARD]    = { SPI_FLASH_PROTOCOL_1S_1S_1S,       0x03, 0 },
        [QSPI_XIP_READ_FAST]        = { SPI_FLASH_PROTOCOL_1S_1S_1S,       0x0B, 8 },
        [QSPI_XIP_READ_DUAL_OUTPUT] = { SPI_FLASH_PROTOCOL_EXTENDED_SPI,   0x00, 0 }, /* unsupported */
        [QSPI_XIP_READ_DUAL_IO]     = { SPI_FLASH_PROTOCOL_1S_2S_2S,      0xBB, 4 },
        [QSPI_XIP_READ_QUAD_OUTPUT] = { SPI_FLASH_PROTOCOL_1S_1S_4S,      0x6B, 8 },
        [QSPI_XIP_READ_QUAD_IO]     = { SPI_FLASH_PROTOCOL_1S_4S_4S,      0xEB, 6 },
    };

    /* The XSPI protocol set has no 1S-1S-2S mode (dual output with a single
     * line address); dual is only available as dual I/O (1S-2S-2S). */
    if (config->read_mode == QSPI_XIP_READ_DUAL_OUTPUT) {
        return QSPI_STATUS_INVALID_PARAMETER;
    }

    ospi_b_xspi_command_set_t *entry = qspi_ospi_entry_for(modes[config->read_mode].protocol);
    if (NULL == entry) {
        return QSPI_STATUS_INVALID_PARAMETER;
    }

    /* Re-aim the memory-mapped read command at the requested format. With
     * the flash in 4-byte addressing mode use the 4-byte read opcodes
     * (0x13/0x0C/0x5C/0xBC/0x6C/0xEC) via the read_command override. */
    entry->address_bytes      = (spi_flash_address_bytes_t)config->address_bytes;
    entry->read_command       = config->read_command ?
                                config->read_command : modes[config->read_mode].default_command;
    entry->read_dummy_cycles  = (config->dummy_clocks == QSPI_XIP_DUMMY_DEFAULT) ?
                                modes[config->read_mode].default_dummy : config->dummy_clocks;

    qspi_status_t status = qspi_ospi_set_protocol(modes[config->read_mode].protocol);
    if (status != QSPI_STATUS_OK) {
        return status;
    }

    /* The first window access after a protocol switch can come back
     * misaligned (stale 1-line data read buffer in the peripheral). Flush
     * it with two throwaway reads at addresses differing in bit 3, the
     * same trick as the FSP r_ospi_b XIP code uses (r_ospi_b_dummy_read). */
    volatile uint64_t dummy = *(volatile uint64_t *)QSPI_XIP_WINDOW_BASE;
    dummy |= *(volatile uint64_t *)(QSPI_XIP_WINDOW_BASE + 8U);
    (void)dummy;

    /* Window only - the flash never enters continuous-read mode, so the
     * command-based API and the window stay usable side by side. */
    obj->xip_exit_command = config->xip_exit_command;
    obj->xip_active = false;

    return QSPI_STATUS_OK;
}

qspi_status_t qspi_xip_exit(qspi_t *obj)
{
    if (obj == NULL) {
        return QSPI_STATUS_INVALID_PARAMETER;
    }

    /* Restore the table to the state the QSPIFBlockDevice fast path expects
     * (4-byte addressing) and re-arm the default protocol. The window keeps
     * working afterwards with whatever format was configured. */
    for (uint32_t i = 0; i < qspi_ospi_cmd_set_table.length; i++) {
        qspi_ospi_cmd_sets[i].address_bytes = SPI_FLASH_ADDRESS_BYTES_4;
    }
    qspi_status_t status = qspi_ospi_set_protocol(SPI_FLASH_PROTOCOL_EXTENDED_SPI);
    if (status != QSPI_STATUS_OK) {
        return status;
    }

    obj->xip_active = false;
    return QSPI_STATUS_OK;
}

#else /* TARGET_RA8P1 */

/* ==========================================================================
 * Legacy RA QSPI peripheral implementation
 * ==========================================================================
 *
 * The RA QSPI controller has no per-phase bus-width sequencer for arbitrary
 * command frames. Instead it offers a byte-wide direct communication mode:
 * every write to SFMCOM shifts 8 bits out on the currently selected number of
 * lines (SFMSPC.SFMSPI = 1, 2 or 4 lines), every read shifts 8 bits in. A full
 * command frame (instruction / address / alternate / dummy / data) is built as
 * a stream of bus "beats" (one clock, W bits, bit j of the beat = IO line j).
 * Phases narrower than the bus width are emulated by placing their bits on the
 * low line(s) and holding the unused lines high, the same trick the FSP
 * r_qspi.c driver uses for the command byte of a quad page program (see
 * qspi_d0_byte_write_quad_mode() there, bits spread at every 4th position with
 * 1s in between).
 *
 * SFMCOM byte bit order (derived from the FSP spread patterns):
 *   - 1-line: plain MSB-first byte (earliest beat = bit 7)
 *   - 2/4-line: bits go out from the MSB down, so the earliest beat occupies
 *     the topmost W-bit group of the byte; within a group, beat bit j = IO j
 *
 * The controller only supports SPI mode 0 (no CPOL/CPHA registers).
 */

#include "qspi_xip.h"

/* Worst-case prefix: instruction (8 bits) + address (32 bits) + alt (32 bits)
 * on a 1-line bus = 72 beats, plus reasonable dummy padding. */
#define QSPI_PRV_MAX_PREFIX_BEATS   104

/* SFMSSC: minimum QSSL high time of 2 QSPCLK (SFMSW = cycles - 1). */
#define QSPI_PRV_MIN_DESELECT       (1U)
#define QSPI_PRV_SFMSSC_DEFAULT     (QSPI_PRV_MIN_DESELECT |                 \
                                     R_QSPI_SFMSSC_SFMSHD_Msk |              \
                                     R_QSPI_SFMSSC_SFMSLD_Msk)

/* SFMSKC divider encoding: 2..17 are contiguous, above 17 only even. */
#define QSPI_PRV_DIV_MIN            2U
#define QSPI_PRV_DIV_MAX            48U

typedef struct {
    uint8_t beats[QSPI_PRV_MAX_PREFIX_BEATS];
    int count;
    int bus_lines;      /* W: number of lines used on the bus (1, 2 or 4) */
} qspi_prv_frame_t;

/* Convert a mbed bus width to a line count. 0 if invalid. */
static int qspi_prv_bus_lines(qspi_bus_width_t width)
{
    switch (width) {
        case QSPI_CFG_BUS_SINGLE:
            return 1;
        case QSPI_CFG_BUS_DUAL:
            return 2;
        case QSPI_CFG_BUS_QUAD:
            return 4;
        default:
            return 0;
    }
}

/* Append nbits (MSB first) of value sent on `lines` lines to the beat stream.
 * Unused bus lines are held high. Returns false if the buffer overflows. */
static bool qspi_prv_append_bits(qspi_prv_frame_t *frame, uint32_t value, int nbits, int lines)
{
    for (int i = nbits - 1; i >= 0;) {
        if (frame->count >= QSPI_PRV_MAX_PREFIX_BEATS) {
            return false;
        }
        uint8_t beat = 0;
        for (int line = 0; line < lines; line++) {
            uint8_t bit = (value >> i) & 1U;
            i--;
            beat |= (uint8_t)(bit << line);
        }
        for (int line = lines; line < frame->bus_lines; line++) {
            beat |= (uint8_t)(1U << line);
        }
        frame->beats[frame->count++] = beat;
    }
    return true;
}

/* Append `cycles` dummy clocks (all lines high). */
static bool qspi_prv_append_dummy(qspi_prv_frame_t *frame, int cycles)
{
    uint8_t beat = 0;
    for (int line = 0; line < frame->bus_lines; line++) {
        beat |= (uint8_t)(1U << line);
    }
    if (frame->count + cycles > QSPI_PRV_MAX_PREFIX_BEATS) {
        return false;
    }
    for (int i = 0; i < cycles; i++) {
        frame->beats[frame->count++] = beat;
    }
    return true;
}

/* Shift the beat stream out through SFMCOM. The beat count must be a multiple
 * of 8/W so the stream ends on a byte boundary.
 *
 * SFMCOM byte layout for multi-line buses: the bits are transmitted from the
 * MSB down, W bits per clock. The first beat therefore occupies the topmost
 * bit group of the byte ([7:4] for quad, [7:6] for dual) and, within each
 * group, bit j of the beat goes to IO line j. This matches the spread
 * patterns in the FSP r_qspi driver (qspi_d0_byte_write_quad_mode places
 * data bit 7 at byte bit 4 and data bit 6 at byte bit 0 of the first byte,
 * i.e. MSB first with the upper nibble transmitted first). */
static void qspi_prv_send_beats(const qspi_prv_frame_t *frame)
{
    int W = frame->bus_lines;
    int beats_per_byte = 8 / W;

    for (int i = 0; i < frame->count; i += beats_per_byte) {
        uint8_t data = 0;
        for (int k = 0; k < beats_per_byte; k++) {
            uint8_t beat = frame->beats[i + k];
            if (W == 1) {
                /* Single line: plain MSB-first byte */
                data |= (uint8_t)(beat << (7 - k));
            } else {
                /* Earliest beat in the highest bit group */
                data |= (uint8_t)(beat << ((beats_per_byte - 1 - k) * W));
            }
        }
        R_QSPI->SFMCOM = data;
    }
}

/* Enter direct communication mode with the given bus width (starts an SPI bus
 * cycle). SFMSPI encoding: 0 = 1 line, 1 = 2 lines, 2 = 4 lines. */
static void qspi_prv_enter_dc(int bus_lines)
{
    R_QSPI->SFMSPC = R_QSPI_SFMSPC_SFMSDE_Msk | (uint32_t)(bus_lines >> 1);
    R_QSPI->SFMCMD = 1U;
}

/* Terminate the current SPI bus cycle (stays in direct communication mode). */
static void qspi_prv_close_cycle(void)
{
    R_QSPI->SFMCMD = 1U;
}

/* Leave direct communication mode (back to ROM access mode). Restore the
 * extended SPI (1-line) protocol: SFMSPI also selects the line count of the
 * instruction phase in the auto-issued read command of the memory-mapped
 * window, so leaving quad/dual protocol set here would corrupt every
 * subsequent window access. This mirrors the restore logic in R_QSPI_Write
 * in the FSP r_qspi driver. */
static void qspi_prv_exit_dc(void)
{
    R_QSPI->SFMCMD = 0U;
    R_QSPI->SFMSPC = R_QSPI_SFMSPC_SFMSDE_Msk;
}

/* True while the controller is in XIP mode. Direct communication is not
 * allowed in this state (see "Using Direct Communication Mode" in the QSPI
 * chapter of the hardware manual). */
static bool qspi_prv_xip_active(void)
{
    return R_QSPI->SFMSDC_b.SFMXST != 0U;
}

/* Trigger a window read and wait for the XIP status bit to reach `expected`.
 * Same sequence as r_qspi_xip() in the FSP r_qspi driver. */
static qspi_status_t qspi_prv_xip_transition(uint8_t code, bool enter)
{
    uint32_t dummy = R_QSPI->SFMSDC & R_QSPI_SFMSDC_SFMDN_Msk;

    R_QSPI->SFMSDC = ((uint32_t)code << R_QSPI_SFMSDC_SFMXD_Pos) |
                     ((uint32_t)enter << R_QSPI_SFMSDC_SFMXEN_Pos) |
                     dummy;

    /* A read from the mapped window sends the XIP enter/exit request. */
    volatile uint8_t trigger = *(volatile uint8_t *)QSPI_XIP_WINDOW_BASE;
    (void)trigger;

    uint32_t timeout = 1000000U;
    while (R_QSPI->SFMSDC_b.SFMXST != (uint32_t)enter) {
        if (--timeout == 0U) {
            return QSPI_STATUS_ERROR;
        }
    }
    return QSPI_STATUS_OK;
}

/* Number of lines the bus must run at: the widest enabled phase. */
static qspi_status_t qspi_prv_frame_bus_width(const qspi_command_t *command, int *bus_lines)
{
    int width = 1;
    int lines;

    if (!command->instruction.disabled) {
        lines = qspi_prv_bus_lines(command->instruction.bus_width);
        if (lines == 0) {
            return QSPI_STATUS_INVALID_PARAMETER;
        }
        if (lines > width) {
            width = lines;
        }
    }
    if (!command->address.disabled) {
        lines = qspi_prv_bus_lines(command->address.bus_width);
        if (lines == 0) {
            return QSPI_STATUS_INVALID_PARAMETER;
        }
        if (lines > width) {
            width = lines;
        }
    }
    if (!command->alt.disabled) {
        lines = qspi_prv_bus_lines(command->alt.bus_width);
        if (lines == 0) {
            return QSPI_STATUS_INVALID_PARAMETER;
        }
        if (lines > width) {
            width = lines;
        }
    }
    lines = qspi_prv_bus_lines(command->data.bus_width);
    if (lines == 0) {
        return QSPI_STATUS_INVALID_PARAMETER;
    }
    if (lines > width) {
        width = lines;
    }

    *bus_lines = width;
    return QSPI_STATUS_OK;
}

/* Build the instruction + address + alternate + dummy prefix of a frame. */
static qspi_status_t qspi_prv_build_prefix(const qspi_command_t *command, qspi_prv_frame_t *frame)
{
    static const int addr_bits[] = { 8, 16, 24, 32 }; /* indexed by qspi_address_size_t */
    int lines;

    frame->count = 0;

    if (!command->instruction.disabled) {
        lines = qspi_prv_bus_lines(command->instruction.bus_width);
        if (!qspi_prv_append_bits(frame, command->instruction.value, 8, lines)) {
            return QSPI_STATUS_INVALID_PARAMETER;
        }
    }
    if (!command->address.disabled) {
        if (command->address.size > QSPI_CFG_ADDR_SIZE_32) {
            return QSPI_STATUS_INVALID_PARAMETER;
        }
        lines = qspi_prv_bus_lines(command->address.bus_width);
        if (!qspi_prv_append_bits(frame, command->address.value,
                                  addr_bits[command->address.size], lines)) {
            return QSPI_STATUS_INVALID_PARAMETER;
        }
    }
    if (!command->alt.disabled) {
        if ((command->alt.size == 0) || (command->alt.size > 32)) {
            return QSPI_STATUS_INVALID_PARAMETER;
        }
        lines = qspi_prv_bus_lines(command->alt.bus_width);
        if (command->alt.size % lines != 0) {
            return QSPI_STATUS_INVALID_PARAMETER;
        }
        if (!qspi_prv_append_bits(frame, command->alt.value, command->alt.size, lines)) {
            return QSPI_STATUS_INVALID_PARAMETER;
        }
    }
    if (command->dummy_count > 0) {
        if (!qspi_prv_append_dummy(frame, command->dummy_count)) {
            return QSPI_STATUS_INVALID_PARAMETER;
        }
    }
    return QSPI_STATUS_OK;
}

/* True if the beat stream ends on an SFMCOM byte boundary (required when a
 * data phase follows in the same bus cycle). */
static bool qspi_prv_byte_aligned(const qspi_prv_frame_t *frame)
{
    return (frame->count % (8 / frame->bus_lines)) == 0;
}

/* Send `length` bytes of data on `lines` lines (lines <= bus width). */
static void qspi_prv_send_data(const uint8_t *data, size_t length, int lines, int bus_lines)
{
    if (lines == bus_lines) {
        /* Data as wide as the bus: SFMCOM byte writes map directly. */
        for (size_t i = 0; i < length; i++) {
            R_QSPI->SFMCOM = data[i];
        }
    } else {
        /* Narrower data phase: spread each byte over beats. */
        for (size_t i = 0; i < length; i++) {
            qspi_prv_frame_t chunk = { .bus_lines = bus_lines, .count = 0 };
            (void)qspi_prv_append_bits(&chunk, data[i], 8, lines);
            qspi_prv_send_beats(&chunk);
        }
    }
}

#if STATIC_PINMAP_READY
#define QSPI_INIT_DIRECT qspi_init_direct
qspi_status_t qspi_init_direct(qspi_t *obj, const qspi_pinmap_t *pinmap, uint32_t hz, uint8_t mode)
#else
#define QSPI_INIT_DIRECT _qspi_init_direct
static qspi_status_t _qspi_init_direct(qspi_t *obj, const qspi_pinmap_t *pinmap, uint32_t hz, uint8_t mode)
#endif
{
    /* The RA QSPI controller only supports SPI mode 0 (no CPOL/CPHA bits). */
    if (mode != 0) {
        return QSPI_STATUS_INVALID_PARAMETER;
    }

    /* Configure the QSPI pins. Note: no pin_mode() here - on RA each
     * R_IOPORT_PinCfg call overwrites the whole PFS register, so calling
     * pin_mode(PullNone) after pin_function() would erase the peripheral
     * mux. Pull-none is already implied by RA_PIN_PULL_NONE. */
    obj->io0 = pinmap->data0_pin;
    pin_function(pinmap->data0_pin, pinmap->data0_function);
    obj->io1 = pinmap->data1_pin;
    pin_function(pinmap->data1_pin, pinmap->data1_function);
    obj->io2 = pinmap->data2_pin;
    pin_function(pinmap->data2_pin, pinmap->data2_function);
    obj->io3 = pinmap->data3_pin;
    pin_function(pinmap->data3_pin, pinmap->data3_function);
    obj->sclk = pinmap->sclk_pin;
    pin_function(pinmap->sclk_pin, pinmap->sclk_function);
    obj->ssel = pinmap->ssel_pin;
    pin_function(pinmap->ssel_pin, pinmap->ssel_function);

    /* Enable the QSPI module clock (resets MSTPCR protection internally). */
    R_BSP_MODULE_START(FSP_IP_QSPI, 0U);

    obj->xip_active = false;
    obj->xip_exit_command = 0U;

    /* Initialize the controller. The ROM access mode settings (SFMSMD, SFMSAC,
     * SFMSDC) are left at their defaults: this HAL only uses direct
     * communication mode and never accesses the memory-mapped window. */
    R_QSPI->SFMCST  = 0U;   /* clear communication status                  */
    R_QSPI->SFMSIC  = 0U;   /* no instruction substitution                 */
    R_QSPI->SFMPMD  = 0U;   /* port control defaults                       */
    R_QSPI->SFMCNT1 = 0U;   /* memory-mapped window base = QSPI address 0  */
    R_QSPI->SFMSMD  = 0U;   /* standard read mode, no prefetch             */
    R_QSPI->SFMSAC  = 0U;   /* 3-byte address (unused in direct mode)      */
    R_QSPI->SFMSDC  = R_QSPI_SFMSDC_SFMXD_Msk;
    R_QSPI->SFMSSC  = QSPI_PRV_SFMSSC_DEFAULT;
    R_QSPI->SFMSPC  = R_QSPI_SFMSPC_SFMSDE_Msk;  /* 1-line extended SPI      */

    return qspi_frequency(obj, hz);
}

qspi_status_t qspi_free(qspi_t *obj)
{
    /* Leave continuous-read mode if it was entered, so the flash is back in
     * a well-known state for the next user. */
    (void)qspi_xip_exit(obj);

    /* Make sure the controller is not left in direct communication mode. */
    qspi_prv_exit_dc();

    /* Disable the QSPI module clock. */
    R_BSP_MODULE_STOP(FSP_IP_QSPI, 0U);

    /* Release the pins (function 0 = analog/reset state). */
    pin_function(obj->io0, 0);
    pin_function(obj->io1, 0);
    pin_function(obj->io2, 0);
    pin_function(obj->io3, 0);
    pin_function(obj->sclk, 0);
    pin_function(obj->ssel, 0);

    return QSPI_STATUS_OK;
}

qspi_status_t qspi_frequency(qspi_t *obj, int hz)
{
    if (hz <= 0) {
        return QSPI_STATUS_INVALID_PARAMETER;
    }

    uint32_t pclka = R_FSP_SystemClockHzGet(FSP_PRIV_CLOCK_PCLKA);

    /* Divider = ceil(PCLKA / hz), clamped to the hardware range. */
    uint32_t div = (pclka + (uint32_t)hz - 1U) / (uint32_t)hz;
    if (div < QSPI_PRV_DIV_MIN) {
        div = QSPI_PRV_DIV_MIN;
    }
    if (div > QSPI_PRV_DIV_MAX) {
        div = QSPI_PRV_DIV_MAX;
    }

    /* Encode into SFMDV: 2..17 contiguous, 18..48 even steps only. */
    uint32_t sfmdv;
    if (div <= 17U) {
        sfmdv = div - 2U;
    } else {
        div = (div + 1U) & ~1U;         /* round up to the next even divider */
        if (div > QSPI_PRV_DIV_MAX) {
            div = QSPI_PRV_DIV_MAX;
        }
        sfmdv = 0x10U + ((div - 18U) / 2U);
    }

    R_QSPI->SFMSKC = sfmdv;
    obj->hz = (int)(pclka / div);

    return QSPI_STATUS_OK;
}

qspi_status_t qspi_write(qspi_t *obj, const qspi_command_t *command, const void *data, size_t *length)
{
    if (qspi_prv_xip_active()) {
        return QSPI_STATUS_ERROR;
    }

    int bus_lines;
    qspi_status_t status = qspi_prv_frame_bus_width(command, &bus_lines);
    if (status != QSPI_STATUS_OK) {
        return status;
    }

    qspi_prv_frame_t frame = { .bus_lines = bus_lines, .count = 0 };
    status = qspi_prv_build_prefix(command, &frame);
    if (status != QSPI_STATUS_OK) {
        return status;
    }

    /* The data phase must start on an SFMCOM byte boundary. */
    if (!qspi_prv_byte_aligned(&frame)) {
        return QSPI_STATUS_INVALID_PARAMETER;
    }

    int data_lines = qspi_prv_bus_lines(command->data.bus_width);

    qspi_prv_enter_dc(bus_lines);
    qspi_prv_send_beats(&frame);
    qspi_prv_send_data((const uint8_t *)data, *length, data_lines, bus_lines);
    qspi_prv_close_cycle();
    qspi_prv_exit_dc();

    return QSPI_STATUS_OK;
}

qspi_status_t qspi_read(qspi_t *obj, const qspi_command_t *command, void *data, size_t *length)
{
    if (qspi_prv_xip_active()) {
        return QSPI_STATUS_ERROR;
    }

    int bus_lines;
    qspi_status_t status = qspi_prv_frame_bus_width(command, &bus_lines);
    if (status != QSPI_STATUS_OK) {
        return status;
    }

    qspi_prv_frame_t frame = { .bus_lines = bus_lines, .count = 0 };
    status = qspi_prv_build_prefix(command, &frame);
    if (status != QSPI_STATUS_OK) {
        return status;
    }

    /* The data phase must start on an SFMCOM byte boundary. */
    if (!qspi_prv_byte_aligned(&frame)) {
        return QSPI_STATUS_INVALID_PARAMETER;
    }

    /* On reads SFMCOM always assembles all bus lines, so data narrower than
     * the bus cannot be captured. */
    if (qspi_prv_bus_lines(command->data.bus_width) != bus_lines) {
        return QSPI_STATUS_INVALID_PARAMETER;
    }

    qspi_prv_enter_dc(bus_lines);
    qspi_prv_send_beats(&frame);
    for (size_t i = 0; i < *length; i++) {
        ((uint8_t *)data)[i] = (uint8_t)R_QSPI->SFMCOM;
    }
    qspi_prv_close_cycle();
    qspi_prv_exit_dc();

    return QSPI_STATUS_OK;
}

qspi_status_t qspi_command_transfer(qspi_t *obj, const qspi_command_t *command,
                                    const void *tx_data, size_t tx_size,
                                    void *rx_data, size_t rx_size)
{
    qspi_status_t status = QSPI_STATUS_OK;

    if (qspi_prv_xip_active()) {
        return QSPI_STATUS_ERROR;
    }

    if ((tx_data == NULL || tx_size == 0) && (rx_data == NULL || rx_size == 0)) {
        /* Command-only frame (e.g. WREN, erase). Pad the beat stream to a byte
         * boundary with idle clocks - trailing clocks are ignored by the
         * flash since no data phase follows. */
        int bus_lines;
        status = qspi_prv_frame_bus_width(command, &bus_lines);
        if (status != QSPI_STATUS_OK) {
            return status;
        }

        qspi_prv_frame_t frame = { .bus_lines = bus_lines, .count = 0 };
        status = qspi_prv_build_prefix(command, &frame);
        if (status != QSPI_STATUS_OK) {
            return status;
        }

        int beats_per_byte = 8 / bus_lines;
        while ((frame.count % beats_per_byte) != 0) {
            if (!qspi_prv_append_dummy(&frame, 1)) {
                return QSPI_STATUS_INVALID_PARAMETER;
            }
        }

        qspi_prv_enter_dc(bus_lines);
        qspi_prv_send_beats(&frame);
        qspi_prv_close_cycle();
        qspi_prv_exit_dc();
    } else {
        /* Transmit first, then receive with the same command (two bus
         * cycles), matching the STM32 implementation. */
        if (tx_data != NULL && tx_size > 0) {
            size_t tx_length = tx_size;
            status = qspi_write(obj, command, tx_data, &tx_length);
            if (status != QSPI_STATUS_OK) {
                return status;
            }
        }

        if (rx_data != NULL && rx_size > 0) {
            size_t rx_length = rx_size;
            status = qspi_read(obj, command, rx_data, &rx_length);
        }
    }
    return status;
}

qspi_status_t qspi_xip_enter(qspi_t *obj, const qspi_xip_config_t *config)
{
    if ((obj == NULL) || (config == NULL)) {
        return QSPI_STATUS_INVALID_PARAMETER;
    }
    if ((config->read_mode > QSPI_XIP_READ_QUAD_IO) ||
            (config->address_bytes < 1U) || (config->address_bytes > 4U)) {
        return QSPI_STATUS_INVALID_PARAMETER;
    }
    if ((config->dummy_clocks != QSPI_XIP_DUMMY_DEFAULT) &&
            (config->dummy_clocks < 3U) && (config->dummy_clocks != 0U)) {
        /* 1 or 2 dummy clocks cannot be expressed in SFMDN (0 selects the
         * hardware default), 3..17 map to SFMDN 1..15. */
        return QSPI_STATUS_INVALID_PARAMETER;
    }
    if (qspi_prv_xip_active()) {
        return QSPI_STATUS_ERROR;
    }

    /* SFMDN encoding: value - 2, 0 keeps the hardware default for the mode
     * (same conversion as R_QSPI_Open in the FSP driver). */
    uint32_t dummy = 0U;
    if ((config->dummy_clocks != QSPI_XIP_DUMMY_DEFAULT) && (config->dummy_clocks != 0U)) {
        dummy = (uint32_t)config->dummy_clocks - 2U;
    }

    /* Address width of the auto-issued read command (SFMAS is 0-based). */
    R_QSPI->SFMSAC = (uint32_t)config->address_bytes - 1U;

    /* Read format (SFMRM) + optional opcode substitution (SFMCCE/SFMSIC) +
     * prefetch. Configured before the XIP trigger read because the trigger
     * read itself uses this format. */
    if (config->read_command != 0U) {
        R_QSPI->SFMSIC = config->read_command;
        R_QSPI->SFMSMD = R_QSPI_SFMSMD_SFMCCE_Msk |
                         (config->prefetch ? R_QSPI_SFMSMD_SFMPFE_Msk : 0U) |
                         (uint32_t)config->read_mode;
    } else {
        R_QSPI->SFMSMD = (config->prefetch ? R_QSPI_SFMSMD_SFMPFE_Msk : 0U) |
                         (uint32_t)config->read_mode;
    }

    obj->xip_exit_command = config->xip_exit_command;

    if (config->xip_enter_command != 0U) {
        qspi_status_t status = qspi_prv_xip_transition(config->xip_enter_command, true);
        if (status != QSPI_STATUS_OK) {
            return status;
        }
        obj->xip_active = true;
    } else {
        /* Window only: make sure no XIP request is pending. */
        R_QSPI->SFMSDC = dummy;
        obj->xip_active = false;
    }

    return QSPI_STATUS_OK;
}

qspi_status_t qspi_xip_exit(qspi_t *obj)
{
    if (obj == NULL) {
        return QSPI_STATUS_INVALID_PARAMETER;
    }

    if (qspi_prv_xip_active()) {
        qspi_status_t status = qspi_prv_xip_transition(obj->xip_exit_command, false);
        if (status != QSPI_STATUS_OK) {
            return status;
        }
    }
    obj->xip_active = false;

    return QSPI_STATUS_OK;
}

#endif /* TARGET_RA8P1 */

/* Generic pinmap-based init, shared by both implementations (the selected
 * branch defines the QSPI_INIT_DIRECT macro above). */
qspi_status_t qspi_init(qspi_t *obj, PinName io0, PinName io1, PinName io2, PinName io3,
                        PinName sclk, PinName ssel, uint32_t hz, uint8_t mode)
{
    QSPIName qspiio0name = (QSPIName)pinmap_peripheral(io0, PinMap_QSPI_DATA0);
    QSPIName qspiio1name = (QSPIName)pinmap_peripheral(io1, PinMap_QSPI_DATA1);
    QSPIName qspiio2name = (QSPIName)pinmap_peripheral(io2, PinMap_QSPI_DATA2);
    QSPIName qspiio3name = (QSPIName)pinmap_peripheral(io3, PinMap_QSPI_DATA3);
    QSPIName qspiclkname = (QSPIName)pinmap_peripheral(sclk, PinMap_QSPI_SCLK);
    QSPIName qspisselname = (QSPIName)pinmap_peripheral(ssel, PinMap_QSPI_SSEL);

    QSPIName qspi_data_first = (QSPIName)pinmap_merge(qspiio0name, qspiio1name);
    QSPIName qspi_data_second = (QSPIName)pinmap_merge(qspiio2name, qspiio3name);
    QSPIName qspi_data_third = (QSPIName)pinmap_merge(qspiclkname, qspisselname);

    if (qspi_data_first != qspi_data_second || qspi_data_second != qspi_data_third ||
            qspi_data_first != qspi_data_third) {
        return QSPI_STATUS_INVALID_PARAMETER;
    }

    int peripheral = (int)qspi_data_first;
    int function_io0 = (int)pinmap_find_function(io0, PinMap_QSPI_DATA0);
    int function_io1 = (int)pinmap_find_function(io1, PinMap_QSPI_DATA1);
    int function_io2 = (int)pinmap_find_function(io2, PinMap_QSPI_DATA2);
    int function_io3 = (int)pinmap_find_function(io3, PinMap_QSPI_DATA3);
    int function_sclk = (int)pinmap_find_function(sclk, PinMap_QSPI_SCLK);
    int function_ssel = (int)pinmap_find_function(ssel, PinMap_QSPI_SSEL);

    const qspi_pinmap_t static_pinmap = {peripheral, io0, function_io0, io1, function_io1,
                                         io2, function_io2, io3, function_io3,
                                         sclk, function_sclk, ssel, function_ssel
                                        };

    return QSPI_INIT_DIRECT(obj, &static_pinmap, hz, mode);
}

const PinMap *qspi_master_sclk_pinmap(void)
{
    return PinMap_QSPI_SCLK;
}

const PinMap *qspi_master_ssel_pinmap(void)
{
    return PinMap_QSPI_SSEL;
}

const PinMap *qspi_master_data0_pinmap(void)
{
    return PinMap_QSPI_DATA0;
}

const PinMap *qspi_master_data1_pinmap(void)
{
    return PinMap_QSPI_DATA1;
}

const PinMap *qspi_master_data2_pinmap(void)
{
    return PinMap_QSPI_DATA2;
}

const PinMap *qspi_master_data3_pinmap(void)
{
    return PinMap_QSPI_DATA3;
}

#endif
