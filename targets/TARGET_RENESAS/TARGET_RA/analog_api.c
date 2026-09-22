/* mbed Microcontroller Library
 * Copyright (c) 2024 ARM Limited
 * SPDX-License-Identifier: Apache-2.0
 */
#include "analogin_api.h"
#include "mbed_assert.h"
#include "mbed_error.h"
#include "hal_data.h"
#include "pinmap.h"
#include "PeripheralPins.h"

#if (BSP_PERIPHERAL_ADC_PRESENT && BSP_PERIPHERAL_ADC_CHANNEL_MASK == 0x3U) || \
    (BSP_PERIPHERAL_ADC_B_PRESENT && BSP_PERIPHERAL_ADC_B_CHANNEL_MASK == 0x3U) || \
    (BSP_PERIPHERAL_ADC_D_PRESENT && BSP_PERIPHERAL_ADC_D_CHANNEL_MASK == 0x3U)
#define HAS_ADC1 1
#endif

static bool g_adc_initialized = false;

#if BSP_PERIPHERAL_ADC_B_PRESENT

/* The ADC_B peripheral has a completely different scan configuration model:
 * physical channels are mapped into per-group "virtual channels" instead of
 * a channel mask. Mbed needs to read arbitrary pins at runtime, so a minimal
 * one-channel scan is re-programmed before every conversion. */

static adc_b_virtual_channel_cfg_t adc_b_vchan_cfg;
static adc_b_virtual_channel_cfg_t * const adc_b_vchan_list[1] = { &adc_b_vchan_cfg };

static adc_b_group_cfg_t adc_b_group_cfg =
{
    .scan_group_id                = ADC_GROUP_ID_0,
    .scan_group_enable            = true,
    .scan_end_interrupt_enable    = false,
    .external_trigger_enable_mask = ADC_B_EXTERNAL_TRIGGER_NONE,
    .elc_trigger_enable_mask      = (elc_peripheral_t) 0,
    .gpt_trigger_enable_mask      = ADC_B_GPT_TRIGGER_NONE,
    .self_diagnosis_mask          = (ADC_B_SELF_DIAGNOSIS_DISABLED << R_ADC_B0_ADSGDCR0_DIAGVAL_Pos),
    .limit_clip_interrupt_enable  = false,
    .virtual_channel_count        = 1,
    .p_virtual_channels           = (adc_b_virtual_channel_cfg_t **) adc_b_vchan_list,
};

static adc_b_group_cfg_t * const adc_b_group_list[1] = { &adc_b_group_cfg };

static const adc_b_scan_cfg_t adc_b_scan_cfg =
{
    .group_count  = 1,
    .p_adc_groups = (adc_b_group_cfg_t **) adc_b_group_list,
};

static uint16_t mbed_adc_b_read(adc_ctrl_t * p_ctrl, adc_b_unit_id_t unit, uint32_t channel)
{
    adc_status_t status;
    uint16_t     adc_value = 0;

    adc_b_vchan_cfg.channel_id                          = ADC_B_VIRTUAL_CHANNEL_0;
    adc_b_vchan_cfg.channel_cfg_bits.group              = 1;
    adc_b_vchan_cfg.channel_cfg_bits.channel            = channel;
    adc_b_vchan_cfg.channel_cfg_bits.differential       = 0;
    adc_b_vchan_cfg.channel_cfg_bits.sample_table_id    = ADC_B_SAMPLING_STATE_TABLE_0;

    adc_b_vchan_cfg.channel_control_a_bits.digital_filter_id = 0;
    adc_b_vchan_cfg.channel_control_a_bits.offset_table_id   = ADC_B_USER_OFFSET_TABLE_SELECTION_DISABLED;
    adc_b_vchan_cfg.channel_control_a_bits.gain_table_id     = ADC_B_USER_GAIN_TABLE_SELECTION_DISABLED;

    adc_b_vchan_cfg.channel_control_b_bits.addition_average_mode  = ADC_B_ADD_AVERAGE_OFF;
    adc_b_vchan_cfg.channel_control_b_bits.addition_average_count = ADC_B_ADD_AVERAGE_1;
    adc_b_vchan_cfg.channel_control_b_bits.compare_match_enable   = false;

    adc_b_vchan_cfg.channel_control_c_bits.limiter_clip_table_id = ADC_B_LIMIT_CLIP_TABLE_SELECTION_NONE;
    adc_b_vchan_cfg.channel_control_c_bits.channel_data_format   = ADC_B_DATA_FORMAT_12_BIT;
    adc_b_vchan_cfg.channel_control_c_bits.data_is_unsigned      = true;

    adc_b_group_cfg.converter_selection = unit;

    fsp_err_t err = R_ADC_B_ScanCfg(p_ctrl, &adc_b_scan_cfg);
    MBED_ASSERT(err == FSP_SUCCESS);

    err = R_ADC_B_ScanStart(p_ctrl);
    MBED_ASSERT(err == FSP_SUCCESS);

    do {
        err = R_ADC_B_StatusGet(p_ctrl, &status);
        MBED_ASSERT(err == FSP_SUCCESS);
    } while (status.state != ADC_STATE_IDLE);

    err = R_ADC_B_Read(p_ctrl, (adc_channel_t) channel, &adc_value);
    /* FSP_ERR_INVALID_DATA means the value is valid but its accuracy is not
     * guaranteed (calibration recommended). Accept it instead of asserting. */
    MBED_ASSERT(err == FSP_SUCCESS || err == FSP_ERR_INVALID_DATA);

    return adc_value;
}

static void mbed_adc_calibrate(adc_ctrl_t * p_ctrl)
{
    fsp_err_t   err    = R_ADC_B_Calibrate(p_ctrl, NULL);
    MBED_ASSERT(err == FSP_SUCCESS);

    /* Calibration takes 24ms - 780ms depending on the configured ADC clock. */
    adc_status_t status;
    do {
        err = R_ADC_B_StatusGet(p_ctrl, &status);
        MBED_ASSERT(err == FSP_SUCCESS);
    } while (status.state != ADC_STATE_IDLE);
}

static void mbed_adc_init_once(void)
{
    if (g_adc_initialized) {
        return;
    }

    fsp_err_t err = R_ADC_B_Open(&g_adc0_ctrl, &g_adc0_cfg);
    MBED_ASSERT(err == FSP_SUCCESS);

#ifdef HAS_ADC1
    /* ADC_B is a single peripheral containing both converter units. Opening
     * the second instance re-programs identical global settings and re-points
     * the shared calibration interrupt context to this control block, so the
     * calibration sequence must run through the instance opened last. */
    err = R_ADC_B_Open(&g_adc1_ctrl, &g_adc1_cfg);
    MBED_ASSERT(err == FSP_SUCCESS);

    mbed_adc_calibrate(&g_adc1_ctrl);
#else
    mbed_adc_calibrate(&g_adc0_ctrl);
#endif

    g_adc_initialized = true;
}

#else /* ADC version A */

static void mbed_adc_init_once(void)
{
    if (g_adc_initialized) {
        return;
    }

    fsp_err_t err;

    err = R_ADC_Open(&g_adc0_ctrl, &g_adc0_cfg);
    MBED_ASSERT(err == FSP_SUCCESS);

    err = R_ADC_ScanCfg(&g_adc0_ctrl, &g_adc0_channel_cfg);
    MBED_ASSERT(err == FSP_SUCCESS);

#ifdef HAS_ADC1
    err = R_ADC_Open(&g_adc1_ctrl, &g_adc1_cfg);
    MBED_ASSERT(err == FSP_SUCCESS);

    err = R_ADC_ScanCfg(&g_adc1_ctrl, &g_adc1_channel_cfg);
    MBED_ASSERT(err == FSP_SUCCESS);
#endif

    g_adc_initialized = true;
}

#endif /* BSP_PERIPHERAL_ADC_B_PRESENT */

void analogin_init(analogin_t *obj, PinName pin)
{
    MBED_ASSERT(obj);

    mbed_adc_init_once();

    uint32_t peripheral = pinmap_peripheral(pin, PinMap_ADC);

    obj->pin = pin;
    obj->peripheral = peripheral;
    uint32_t function = pinmap_function(pin, PinMap_ADC);
    pin_function(pin, function);
    obj->channel = RA_PIN_CHANNEL(function);
}

void analogin_free(analogin_t *obj)
{
    pin_mode(obj->pin, PullNone);

    obj->pin = NC;
    obj->peripheral = 0;
    obj->channel = 0;
}

float analogin_read(analogin_t *obj)
{
    uint16_t value = analogin_read_u16(obj);
    return (float)value / 65535.0f;
}

uint16_t analogin_read_u16(analogin_t *obj)
{
    uint16_t adc_value = 0;

#if BSP_PERIPHERAL_ADC_B_PRESENT
    if (obj->peripheral == ADC_0)
    {
        adc_value = mbed_adc_b_read(&g_adc0_ctrl, (adc_b_unit_id_t) 0, obj->channel);
    }
#ifdef HAS_ADC1
    else if (obj->peripheral == ADC_1)
    {
        adc_value = mbed_adc_b_read(&g_adc1_ctrl, (adc_b_unit_id_t) 1, obj->channel);
    }
#endif
#else
    if(obj->peripheral == ADC_0)
    {
        fsp_err_t err = R_ADC_ScanStart(&g_adc0_ctrl);
        MBED_ASSERT(err == FSP_SUCCESS);

        adc_status_t status;
        do {
            R_ADC_StatusGet(&g_adc0_ctrl, &status);
        } while (status.state != ADC_STATE_IDLE);

        err = R_ADC_Read(&g_adc0_ctrl, (adc_channel_t) obj->channel, &adc_value);
        MBED_ASSERT(err == FSP_SUCCESS);
    }
#ifdef HAS_ADC1
    else if(obj->peripheral == ADC_1)
    {
        fsp_err_t err = R_ADC_ScanStart(&g_adc1_ctrl);
        MBED_ASSERT(err == FSP_SUCCESS);

        adc_status_t status;
        do {
            R_ADC_StatusGet(&g_adc1_ctrl, &status);
        } while (status.state != ADC_STATE_IDLE);

        err = R_ADC_Read(&g_adc1_ctrl, (adc_channel_t) obj->channel, &adc_value);
        MBED_ASSERT(err == FSP_SUCCESS);
    }
#endif
#endif
    // Extend from 12bits to 16bits
    return (adc_value << 4);
}

PinName analogin_pin(analogin_t *obj)
{
    return obj->pin;
}

const PinMap *analogin_pinmap(void)
{
    return PinMap_ADC;
}
