/* mbed Microcontroller Library
 * Copyright (c) 2024 ARM Limited
 * SPDX-License-Identifier: Apache-2.0
 */
#include "gpio_irq_api.h"
#include "pinmap.h"
#include "mbed_critical.h"
#include "mbed_assert.h"
#include "common_data.h"
#include "PeripheralPins.h"

#ifdef __cplusplus
extern "C" {
#endif

static gpio_irq_handler s_handler = NULL;
static uint8_t          s_irq_used[IRQ_CHANNELS_COUNT] = {0};
static gpio_irq_t *     s_irq_obj[IRQ_CHANNELS_COUNT] = {0};

extern icu_instance_ctrl_t * const g_icu_ctrl[IRQ_CHANNELS_COUNT];
extern const external_irq_cfg_t * const g_icu_cfg[IRQ_CHANNELS_COUNT];

/* Hooks for targets that assign ICU vector table slots to IRQ channels
 * dynamically (RA8P1: only a few vector entries are reserved for external
 * IRQs and linked to a channel through IELSR on demand, see
 * TARGET_RA8P1/gpio_irq_device.c). The default implementations are no-ops:
 * on targets with a fixed channel-to-vector mapping the generated
 * configuration is used as-is. */
__WEAK int ra_gpio_irq_vector_acquire(uint32_t channel)
{
    (void)channel;
    return 0;
}

__WEAK void ra_gpio_irq_vector_release(uint32_t channel)
{
    (void)channel;
}

void external_irq_callback(external_irq_callback_args_t *p_args)
{
    uint32_t ch = p_args->channel;

    if (ch < IRQ_CHANNELS_COUNT && s_irq_used[ch] && s_handler)
    {
        gpio_irq_t *obj = s_irq_obj[ch];
        int level = R_BSP_PinRead((bsp_io_port_pin_t) obj->pin);

        gpio_irq_event evt = level ? IRQ_RISE : IRQ_FALL;

        s_handler(obj->context, evt);
    }
}

int gpio_irq_init(gpio_irq_t *obj, PinName pin, gpio_irq_handler handler, uintptr_t context)
{
    MBED_ASSERT(obj);
    MBED_ASSERT(pin != NC);

    core_util_critical_section_enter();
    s_handler = handler;
    core_util_critical_section_exit();

    int irq_channel = pinmap_peripheral(pin, PinMap_IRQ);
    MBED_ASSERT(irq_channel != (int)NC);

    /* Link the channel to a vector table slot reserved for external IRQs
     * (dynamic allocation on RA8P1, no-op elsewhere). */
    if (ra_gpio_irq_vector_acquire((uint32_t) irq_channel) != 0)
        return -1;

    int irq_function = pinmap_function(pin, PinMap_IRQ);
    pin_function(pin, irq_function);

    obj->pin = pin;
    obj->channel  = irq_channel;
    obj->context  = context;

    s_irq_obj[irq_channel] = obj;
    s_irq_used[irq_channel] = 1;

    fsp_err_t err = R_ICU_ExternalIrqOpen(g_icu_ctrl[irq_channel], g_icu_cfg[irq_channel]);
    MBED_ASSERT(err == FSP_SUCCESS);

    R_ICU_ExternalIrqEnable(g_icu_ctrl[irq_channel]);

    return 0;
}

void gpio_irq_free(gpio_irq_t *obj)
{
    MBED_ASSERT(obj);

    uint32_t ch = obj->channel;
    if (ch >= IRQ_CHANNELS_COUNT)
        return;

    s_irq_used[ch] = 0;

    R_ICU_ExternalIrqDisable(g_icu_ctrl[ch]);
    R_ICU_ExternalIrqClose(g_icu_ctrl[ch]);

    /* Give the vector table slot of this channel back to the pool
     * (dynamic allocation on RA8P1, no-op elsewhere). */
    ra_gpio_irq_vector_release(ch);

    obj->pin = NC;
}

void gpio_irq_set(gpio_irq_t *obj, gpio_irq_event event, uint32_t enable)
{
    (void)obj;
    (void)event;
    (void)enable;
}

void gpio_irq_enable(gpio_irq_t *obj)
{
    R_ICU_ExternalIrqEnable(g_icu_ctrl[obj->channel]);
}

void gpio_irq_disable(gpio_irq_t *obj)
{
    R_ICU_ExternalIrqDisable(g_icu_ctrl[obj->channel]);
}

const PinMap *gpio_irq_pinmap(void)
{
    return PinMap_IRQ;
}

#ifdef __cplusplus
}
#endif
