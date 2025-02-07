/* mbed Microcontroller Library
 * Copyright (c) 2006-2013 ARM Limited
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "PeripheralPins.h"

// Note on MIMXRT pin functions:
// The MIMXRT's pin function system is pretty dang complicated, and Mbed's pin function data for this chip
// (the 3rd element in each pinmap entry) has to be pretty complicated to match.
// It's a 32-bit bitfield with the following format:
// __________________________________________________________________________________________________
// |                               |                            |              |                     |
// |  Daisy Reg Value (bits 19:16) | Daisy Register (bits 15:4) | SION (bit 3) | Mux Mode (bits 2:0) |
// |_______________________________|____________________________|______________|_____________________|
//
// Mux mode:
//    This value gets written to the IOMUXC_SW_MUX_CTL_PAD_xxx.MUX_MODE bitfield for the given pin.
//    It's a number from 0 to 7 that selects the possible mux mode.
//    See Table 10-1 in the datasheet for the possible muxing options
//
// SION:
//    This is a somewhat unusual setting used to "force the pin mode to input regardless of MUX_MODE
//    functionality".  It's a setting needed for certain peripherals to work that use pins in input mode.
//    I'm not quite sure of the logic for when to use it...
//
// Daisy Register:
//    If nonzero, this field specifies the offset for a "daisy chain register" to use when setting up the pin
//    function.  "Daisy chain" is actually kinda a misnomer, this register is used to select which of multiple
//    pin options a peripheral is connected to, it doesn't daisy chain pins together.  It would be better to
//    call it "top-level mux register" or something.
//
//    The daisy register is specified as an offset relative to the IOMUXC peripheral base.  For example, for the
//    LPI2C3 peripheral, the daisy chain register for SCL is IOMUXC_LPI2C3_SDA_SELECT_INPUT.  So, since the address
//    of that register is IOMUXC + 0x4E0, I'd put 0x4E0 as the daisy register.
//
// Daisy Reg Value:
//    Numeric option to select in the above daisy register, if the address is given.
//

/************RTC***************/
const PinMap PinMap_RTC[] = {
    {NC, OSC32KCLK, 0},
};

/************ADC***************/
const PinMap PinMap_ADC[] = {
    {GPIO_AD_00, ADC1_0, 5},
    {GPIO_AD_01, ADC1_1, 5},
    {GPIO_AD_02, ADC1_2, 5},
    {GPIO_AD_03, ADC1_3, 5},
    {GPIO_AD_04, ADC1_4, 5},
    {GPIO_AD_05, ADC1_5, 5},
    {GPIO_AD_06, ADC1_6, 5},
    {GPIO_AD_07, ADC1_7, 5},
    {GPIO_AD_08, ADC1_8, 5},
    {GPIO_AD_09, ADC1_9, 5},
    {GPIO_AD_10, ADC1_10, 5},
    {GPIO_AD_11, ADC1_11, 5},
    {GPIO_AD_12, ADC1_12, 5},
    {GPIO_AD_13, ADC1_13, 5},
    {GPIO_AD_14, ADC1_14, 5},
    {NC   , NC       , 0}
};

/************DAC***************/
const PinMap PinMap_DAC[] = {
    {NC      , NC   , 0}
};

/************I2C***************/
const PinMap PinMap_I2C_SDA[] = {
    {GPIO_01, I2C_1, ((3U << DAISY_REG_VALUE_SHIFT) | (0x1C4 << DAISY_REG_SHIFT) | (1U << SION_BIT_SHIFT) | 3)},
    {NC   , NC   , 0}
};

const PinMap PinMap_I2C_SCL[] = {
    {GPIO_02, I2C_1, ((3U << DAISY_REG_VALUE_SHIFT) | (0x1C8 << DAISY_REG_SHIFT) | (1U << SION_BIT_SHIFT) | 3)},
    {NC   , NC   , 0}
};

/************UART***************/
const PinMap PinMap_UART_TX[] = {
    {GPIO_10, UART_1, ((1 << DAISY_REG_VALUE_SHIFT) | (0x1F4 << DAISY_REG_SHIFT) | 0)},
    {NC  ,  NC    , 0}
};

const PinMap PinMap_UART_RX[] = {
    {GPIO_09, UART_1, ((1 << DAISY_REG_VALUE_SHIFT) | (0x1F0 << DAISY_REG_SHIFT) | 0)},
    {NC  ,  NC    , 0}
};

/************SPI***************/
const PinMap PinMap_SPI_SCLK[] = {
    {GPIO_AD_06, SPI_1, ((0U << DAISY_REG_VALUE_SHIFT) | (0x1D4 << DAISY_REG_SHIFT) | 0)},
    {NC   , NC   , 0}
};

const PinMap PinMap_SPI_MOSI[] = {
    {GPIO_AD_04, SPI_1, ((0U << DAISY_REG_VALUE_SHIFT) | (0x1DC << DAISY_REG_SHIFT) | 0)},
    {NC   , NC   , 0}
};

const PinMap PinMap_SPI_MISO[] = {
    {GPIO_AD_03, SPI_1, ((0U << DAISY_REG_VALUE_SHIFT) | (0x1D8 << DAISY_REG_SHIFT) | 0)},
    {NC   , NC   , 0}
};

const PinMap PinMap_SPI_SSEL[] = {
    {GPIO_AD_05, SPI_1, ((0U << DAISY_REG_VALUE_SHIFT) | (0x1D0 << DAISY_REG_SHIFT) | 0)},
    {NC   , NC   , 0}
};

/************PWM***************/
const PinMap PinMap_PWM[] = {
    {GPIO_AD_06, PWM_10, ((0U << DAISY_REG_VALUE_SHIFT) | (0x180 << DAISY_REG_SHIFT) | 2)},
    {GPIO_AD_05, PWM_11, ((0U << DAISY_REG_VALUE_SHIFT) | (0x190 << DAISY_REG_SHIFT) | 2)},
    {GPIO_01, PWM_2, ((1U << DAISY_REG_VALUE_SHIFT) | (0x184 << DAISY_REG_SHIFT) | 2)},
    {GPIO_02, PWM_1, ((1U << DAISY_REG_VALUE_SHIFT) | (0x174 << DAISY_REG_SHIFT) | 2)},
    {GPIO_04, PWM_4, ((1U << DAISY_REG_VALUE_SHIFT) | (0x178 << DAISY_REG_SHIFT) | 2)},
    {GPIO_06, PWM_7, ((1U << DAISY_REG_VALUE_SHIFT) | (0x17C << DAISY_REG_SHIFT) | 2)},
    {NC   , NC    , 0}
};
