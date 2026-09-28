/* mbed Microcontroller Library
 * Copyright (c) 2026 ARM Limited
 * SPDX-License-Identifier: Apache-2.0
 *
 * Renesas RA QSPI XIP / memory-mapped extension.
 *
 * This header is a target-specific extension to the standard mbed QSPI HAL
 * (hal/qspi_api.h); it does not replace or modify it. It exposes the RA QSPI
 * memory-mapped window (ROM access mode) and the optional XIP (continuous
 * read) mode on top of an initialized qspi_t object.
 *
 * While the window is configured, reads from QSPI_XIP_WINDOW_BASE are
 * converted by the QSPI controller into the configured serial read command
 * (e.g. fast read quad I/O) with the hardware prefetch buffer. The
 * command-based API (qspi_read/qspi_write/qspi_command_transfer) keeps
 * working unless true XIP mode is active, in which case those functions
 * return QSPI_STATUS_ERROR until qspi_xip_exit() is called.
 *
 * Typical usage:
 *
 *     qspi_t qspi;
 *     qspi_init(&qspi, ...);
 *
 *     // map the window as a fast quad read (Winbond W25Q64JV, 6Bh, 8 dummies)
 *     const qspi_xip_config_t cfg = {
 *         .read_mode = QSPI_XIP_READ_QUAD_OUTPUT,
 *         .address_bytes = 3,
 *         .dummy_clocks = 8,
 *         .xip_enter_command = 0,   // window only, no continuous-read mode
 *     };
 *     qspi_xip_enter(&qspi, &cfg);
 *     uint8_t buf[16];
 *     memcpy(buf, (const void *)QSPI_XIP_WINDOW_BASE, sizeof(buf));
 *     ...
 *     qspi_xip_exit(&qspi);   // back to command-only operation
 *
 * True XIP (flash continuous-read) additionally requires a read format with
 * a mode/alternate byte phase (QSPI_XIP_READ_DUAL_IO or _QUAD_IO). The
 * enter/exit values are flash specific, e.g. for Winbond W25Q in EBh mode:
 * xip_enter_command = 0xA5 (M = A5h keeps continuous read),
 * xip_exit_command  = 0xFF (any value other than A5h leaves it).
 */

#ifndef MBED_QSPI_XIP_H
#define MBED_QSPI_XIP_H

#include "qspi_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Selects the serial read command the controller issues for window accesses.
 * Values match the hardware SFMRM encoding; the default opcode of each mode
 * is given in brackets and can be overridden with read_command. */
typedef enum {
    QSPI_XIP_READ_STANDARD    = 0,  /* standard read [03h], no dummy          */
    QSPI_XIP_READ_FAST        = 1,  /* fast read [0Bh], 1-1-1 + dummy         */
    QSPI_XIP_READ_DUAL_OUTPUT = 2,  /* fast read dual output [3Bh]            */
    QSPI_XIP_READ_DUAL_IO     = 3,  /* fast read dual I/O [BBh] + mode byte   */
    QSPI_XIP_READ_QUAD_OUTPUT = 4,  /* fast read quad output [6Bh]            */
    QSPI_XIP_READ_QUAD_IO     = 5,  /* fast read quad I/O [EBh] + mode byte   */
} qspi_xip_read_mode_t;

/* dummy_clocks sentinel: use the hardware default count for the mode. */
#define QSPI_XIP_DUMMY_DEFAULT  0xFF

typedef struct {
    qspi_xip_read_mode_t read_mode;     /* serial read format of the window   */
    uint8_t address_bytes;              /* 1..4 (typically 3)                 */
    uint8_t dummy_clocks;               /* 3..17, or 0/QSPI_XIP_DUMMY_DEFAULT
                                         * for the mode default (values 1 and 2
                                         * cannot be expressed by the hardware) */
    uint8_t read_command;               /* opcode override, 0 = mode default  */
    uint8_t xip_enter_command;          /* continuous-read enter value, 0 = map
                                         * the window without entering XIP    */
    uint8_t xip_exit_command;           /* value that leaves continuous read  */
    bool prefetch;                      /* enable the prefetch buffer         */
} qspi_xip_config_t;

/** Configure the memory-mapped window and optionally enter XIP mode.
 *
 * The qspi_t object must have been initialized with qspi_init(). Must not be
 * in XIP mode already (call qspi_xip_exit() first).
 *
 * @return QSPI_STATUS_OK on success
 *         QSPI_STATUS_INVALID_PARAMETER on bad configuration
 *         QSPI_STATUS_ERROR if already in XIP mode or the controller does not
 *         confirm the mode change
 */
qspi_status_t qspi_xip_enter(qspi_t *obj, const qspi_xip_config_t *config);

/** Leave XIP mode (if active).
 *
 * The memory-mapped window keeps working with the configured read format
 * afterwards; this only disables continuous-read mode so that the
 * command-based API (qspi_read/qspi_write/qspi_command_transfer) can be
 * used again.
 *
 * @return QSPI_STATUS_OK on success, QSPI_STATUS_ERROR on timeout
 */
qspi_status_t qspi_xip_exit(qspi_t *obj);

#ifdef __cplusplus
}
#endif

#endif /* MBED_QSPI_XIP_H */
