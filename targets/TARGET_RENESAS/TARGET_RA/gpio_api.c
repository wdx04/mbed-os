/* mbed Microcontroller Library
 * Copyright (c) 2024 ARM Limited
 * SPDX-License-Identifier: Apache-2.0
 */
#include "gpio_api.h"
#include "PinNamesTypes.h"
#include "r_ioport.h"
#include "common_data.h"

static inline bsp_io_port_pin_t pin_to_bsp(PinName pin)
{
    return (bsp_io_port_pin_t)pin;
}

static inline bsp_io_port_t pin_to_port(PinName pin)
{
    return (bsp_io_port_t)RA_PORT(pin);
}

static inline uint32_t pin_to_mask(PinName pin)
{
    return (1U << RA_PIN(pin));
}

void gpio_init(gpio_t *obj, PinName pin)
{
    obj->pin = pin;

    /* An unconnected pin (NC) must not touch the PFS registers: computing
     * PORT[RA_PORT(NC)].PIN[RA_PIN(NC)] yields an unmapped address and
     * bus-faults. gpio_is_connected() reports the pin as not connected. */
    if (pin == NC) {
        obj->port = 0;
        obj->pin_mask = 0;
        return;
    }

    obj->port = pin_to_port(pin);
    obj->pin_mask = pin_to_mask(pin);

    // Keep IRQ flag
#if (3U == BSP_FEATURE_IOPORT_VERSION)
    uint32_t cfg = R_PFS->PORT[RA_PORT(pin)].PIN[RA_PIN(pin)].PmnPFS;
#else
    uint32_t cfg = R_PFS->PORT[RA_PORT(pin)].PIN[RA_PIN(pin)].PmnPFS;
#endif
    R_BSP_PinCfg(pin_to_bsp(pin), (cfg & IOPORT_CFG_IRQ_ENABLE) ? IOPORT_CFG_IRQ_ENABLE: IOPORT_CFG_PORT_DIRECTION_INPUT);
}

void gpio_mode(gpio_t *obj, PinMode mode)
{
    PinName pin = obj->pin;
    if (pin == NC) {
        return;
    }
#if (3U == BSP_FEATURE_IOPORT_VERSION)
    uint32_t cfg = R_PFS->PORT[RA_PORT(pin)].PIN[RA_PIN(pin)].PmnPFS;
#else
    uint32_t cfg = R_PFS->PORT[RA_PORT(pin)].PIN[RA_PIN(pin)].PmnPFS;
#endif

    switch (mode) {
        case PullUp:
            cfg |= IOPORT_CFG_PULLUP_ENABLE;
            cfg &= ~IOPORT_CFG_NMOS_ENABLE;
            break;
        case OpenDrain:
            cfg &= ~IOPORT_CFG_PULLUP_ENABLE;
            cfg |= IOPORT_CFG_NMOS_ENABLE;
            break;
        case PullDown:
        case PullNone:
        default:
            cfg &= ~(IOPORT_CFG_PULLUP_ENABLE|IOPORT_CFG_NMOS_ENABLE);
            break;
    }

    R_BSP_PinCfg(pin_to_bsp(pin), cfg);
}

void gpio_dir(gpio_t *obj, PinDirection direction)
{
    PinName pin = obj->pin;
    if (pin == NC) {
        return;
    }
#if (3U == BSP_FEATURE_IOPORT_VERSION)
    uint32_t cfg = R_PFS->PORT[RA_PORT(pin)].PIN[RA_PIN(pin)].PmnPFS;
#else
    uint32_t cfg = R_PFS->PORT[RA_PORT(pin)].PIN[RA_PIN(pin)].PmnPFS;
#endif

    if (direction == PIN_OUTPUT) {
        cfg |= IOPORT_CFG_PORT_DIRECTION_OUTPUT;
    } else {
        cfg &= ~IOPORT_CFG_PORT_DIRECTION_OUTPUT;
    }

    R_BSP_PinCfg(pin_to_bsp(obj->pin), cfg);
}

void gpio_write(gpio_t *obj, int value)
{
    if (obj->pin == NC) {
        return;
    }
    R_BSP_PinWrite(pin_to_bsp(obj->pin), value ? BSP_IO_LEVEL_HIGH : BSP_IO_LEVEL_LOW);
}

int gpio_read(gpio_t *obj)
{
    if (obj->pin == NC) {
        return 0;
    }
    return (R_BSP_PinRead(pin_to_bsp(obj->pin)) != 0) ? 1 : 0;
}

int gpio_is_connected(const gpio_t *obj)
{
    if (obj->pin == NC) {
        return 0;
    } else {
        return 1;
    }
}
