/* mbed Microcontroller Library
 * Copyright (c) 2024 ARM Limited
 * SPDX-License-Identifier: Apache-2.0
 */
#include "analogout_api.h"
#include "pinmap.h"
#include "mbed_assert.h"
#include "objects.h"
#include "PeripheralPins.h"

#if BSP_PERIPHERAL_DAC_B_PRESENT
#define DAC_OPEN   R_DAC_B_Open
#define DAC_WRITE  R_DAC_B_Write
#define DAC_START  R_DAC_B_Start
#else
#define DAC_OPEN   R_DAC_Open
#define DAC_WRITE  R_DAC_Write
#define DAC_START  R_DAC_Start
#endif

static dac_instance_ctrl_t dac_ctrls[2];

void analogout_init(dac_t *obj, PinName pin)
{
    MBED_ASSERT(obj);

    int ch = pinmap_peripheral(pin, PinMap_DAC);
    MBED_ASSERT(ch != NC);

    obj->channel = ch;
    obj->ctrl = &dac_ctrls[ch];

    pinmap_pinout(pin, PinMap_DAC);

#if BSP_PERIPHERAL_DAC_B_PRESENT
    obj->ext_cfg.internal_output_enabled = false;
    obj->ext_cfg.data_format = DAC_DATA_FORMAT_FLUSH_RIGHT;
    obj->ext_cfg.vrefh = DAC_B_VREFH_NORMAL;
#else
    obj->ext_cfg.enable_charge_pump = true;
    obj->ext_cfg.output_amplifier_enabled = true;
    obj->ext_cfg.internal_output_enabled = false;
    obj->ext_cfg.data_format = DAC_DATA_FORMAT_FLUSH_RIGHT;
    obj->ext_cfg.ref_volt_sel = DAC_VREF_VREFH_VREFL;
#endif

    obj->cfg.channel = ch;
    obj->cfg.ad_da_synchronized = false;
    obj->cfg.p_extend = &obj->ext_cfg;

    DAC_OPEN(obj->ctrl, &obj->cfg);

    obj->last_value = 0;
    DAC_WRITE(obj->ctrl, 0);

    DAC_START(obj->ctrl);
}

void analogout_free(dac_t *obj)
{
}

void analogout_write_u16(dac_t *obj, uint16_t value)
{
    obj->last_value = value;

    uint16_t dac_val = value >> 4;

    DAC_WRITE(obj->ctrl, dac_val);
}

uint16_t analogout_read_u16(dac_t *obj)
{
    return obj->last_value;
}

void analogout_write(dac_t *obj, float value)
{
    if (value < 0.0f) value = 0.0f;
    if (value > 1.0f) value = 1.0f;

    uint16_t v = (uint16_t)(value * 65535.0f);
    analogout_write_u16(obj, v);
}

float analogout_read(dac_t *obj)
{
    return analogout_read_u16(obj) / 65535.0f;
}

const PinMap *analogout_pinmap()
{
    return PinMap_DAC;
}
