/* mbed Microcontroller Library
 * Copyright (c) 2026 ARM Limited
 * SPDX-License-Identifier: Apache-2.0
 *
 * QSPI HAL implementation for the Renesas RA QSPI peripheral.
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

#if DEVICE_QSPI

#include "qspi_api.h"
#include "qspi_xip.h"
#include "mbed_error.h"
#include "pinmap.h"
#include "PeripheralPins.h"
#include "objects.h"
#include "bsp_api.h"

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
