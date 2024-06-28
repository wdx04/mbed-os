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
    {GPIO_AD_B1_10, ADC1_10, 5},
    {GPIO_AD_B1_11, ADC1_11, 5},
    {GPIO_AD_B1_12, ADC1_12, 5},
    {GPIO_AD_B1_13, ADC1_13, 5},
    {GPIO_AD_B1_15, ADC1_15, 5},
    {GPIO_AD_B1_14, ADC1_14, 5},
    {NC   , NC       , 0}
};

/************DAC***************/
const PinMap PinMap_DAC[] = {
    {NC      , NC   , 0}
};

/************I2C***************/
const PinMap PinMap_I2C_SDA[] = {
    {GPIO_AD_B1_15, I2C_1, ((1U << DAISY_REG_VALUE_SHIFT) | (0x380 << DAISY_REG_SHIFT) | (1U << SION_BIT_SHIFT) | 0)},
    {GPIO_AD_B1_09, I2C_2, ((0U << DAISY_REG_VALUE_SHIFT) | (0x388 << DAISY_REG_SHIFT) | (1U << SION_BIT_SHIFT) | 0)},
    {GPIO_SD_B1_03, I2C_4, ((1U << DAISY_REG_VALUE_SHIFT) | (0x398 << DAISY_REG_SHIFT) | (1U << SION_BIT_SHIFT) | 3)},
    {NC   , NC   , 0}
};

const PinMap PinMap_I2C_SCL[] = {
    {GPIO_AD_B1_14, I2C_1, ((1U << DAISY_REG_VALUE_SHIFT) | (0x37C << DAISY_REG_SHIFT) | (1U << SION_BIT_SHIFT) | 0)},
    {GPIO_AD_B1_08, I2C_2, ((0U << DAISY_REG_VALUE_SHIFT) | (0x384 << DAISY_REG_SHIFT) | (1U << SION_BIT_SHIFT) | 0)},
    {GPIO_SD_B1_02, I2C_4, ((1U << DAISY_REG_VALUE_SHIFT) | (0x394 << DAISY_REG_SHIFT) | (1U << SION_BIT_SHIFT) | 3)},
    {NC   , NC   , 0}
};

/************UART***************/
const PinMap PinMap_UART_TX[] = {
    {GPIO_AD_B0_06, UART_1, 2},
    {GPIO_AD_B1_08, UART_2, ((0 << DAISY_REG_VALUE_SHIFT) | (0x3D4 << DAISY_REG_SHIFT) | 2)},
    {GPIO_AD_B0_14, UART_3, ((1 << DAISY_REG_VALUE_SHIFT) | (0x3DC << DAISY_REG_SHIFT) | 2)},
    {GPIO_AD_B1_10, UART_4, ((1 << DAISY_REG_VALUE_SHIFT) | (0x3E8 << DAISY_REG_SHIFT) | 2)},
    {GPIO_AD_B0_10, UART_5, ((0 << DAISY_REG_VALUE_SHIFT) | (0x3F0 << DAISY_REG_SHIFT) | 2)},
    {NC  ,  NC    , 0}
};

const PinMap PinMap_UART_RX[] = {
    {GPIO_AD_B0_07, UART_1, 2},
    {GPIO_AD_B1_09, UART_2, ((0 << DAISY_REG_VALUE_SHIFT) | (0x3D0 << DAISY_REG_SHIFT) | 2)},
    {GPIO_AD_B0_15, UART_3, ((1 << DAISY_REG_VALUE_SHIFT) | (0x3D8 << DAISY_REG_SHIFT) | 2)},
    {GPIO_AD_B1_11, UART_4, ((1 << DAISY_REG_VALUE_SHIFT) | (0x3E4 << DAISY_REG_SHIFT) | 2)},
    {GPIO_AD_B0_11, UART_5, ((0 << DAISY_REG_VALUE_SHIFT) | (0x3EC << DAISY_REG_SHIFT) | 2)},
    {NC  ,  NC    , 0}
};

/************SPI***************/
const PinMap PinMap_SPI_SCLK[] = {
    {GPIO_AD_B0_10, SPI_1, ((1U << DAISY_REG_VALUE_SHIFT) | (0x3A0 << DAISY_REG_SHIFT) | 1)},
    {GPIO_AD_B1_12, SPI_3, 2},
    {NC   , NC   , 0}
};

const PinMap PinMap_SPI_MOSI[] = {
    {GPIO_AD_B0_12, SPI_1, ((1U << DAISY_REG_VALUE_SHIFT) | (0x3A8 << DAISY_REG_SHIFT) | 1)},
    {GPIO_AD_B1_14, SPI_3, 2},
    {NC   , NC   , 0}
};

const PinMap PinMap_SPI_MISO[] = {
    {GPIO_AD_B0_13, SPI_1, ((1U << DAISY_REG_VALUE_SHIFT) | (0x3A4 << DAISY_REG_SHIFT) | 1)},
    {GPIO_AD_B1_15, SPI_3, 2},
    {NC   , NC   , 0}
};

const PinMap PinMap_SPI_SSEL[] = {
    {GPIO_AD_B0_11, SPI_1, ((1U << DAISY_REG_VALUE_SHIFT) | (0x39C << DAISY_REG_SHIFT) | 1)},
    {GPIO_AD_B1_13, SPI_3, 2},
    {NC   , NC   , 0}
};

/************PWM***************/
const PinMap PinMap_PWM[] = {
    {GPIO_AD_B0_06, PWM_22, ((0U << DAISY_REG_VALUE_SHIFT) | (0x354 << DAISY_REG_SHIFT) | 4)},
    {GPIO_AD_B0_07, PWM_23, ((0U << DAISY_REG_VALUE_SHIFT) | (0x364 << DAISY_REG_SHIFT) | 4)},
    {GPIO_AD_B0_10, PWM_19, ((0U << DAISY_REG_VALUE_SHIFT) | (0x350 << DAISY_REG_SHIFT) | 4)},
    {GPIO_AD_B0_11, PWM_20, ((0U << DAISY_REG_VALUE_SHIFT) | (0x360 << DAISY_REG_SHIFT) | 4)},
    {GPIO_AD_B0_12, PWM_16, ((0U << DAISY_REG_VALUE_SHIFT) | (0x34C << DAISY_REG_SHIFT) | 4)},
    {GPIO_AD_B0_13, PWM_17, ((0U << DAISY_REG_VALUE_SHIFT) | (0x35C << DAISY_REG_SHIFT) | 4)},
    {GPIO_AD_B0_14, PWM_13, ((0U << DAISY_REG_VALUE_SHIFT) | (0x348 << DAISY_REG_SHIFT) | 4)},
    {GPIO_AD_B0_15, PWM_14, ((0U << DAISY_REG_VALUE_SHIFT) | (0x358 << DAISY_REG_SHIFT) | 4)},
    {GPIO_AD_B1_06, PWM_1,  ((0U << DAISY_REG_VALUE_SHIFT) | (0x328 << DAISY_REG_SHIFT) | 1)},
    {GPIO_AD_B1_07, PWM_2,  ((0U << DAISY_REG_VALUE_SHIFT) | (0x338 << DAISY_REG_SHIFT) | 1)},
    {GPIO_AD_B1_08, PWM_4,  ((0U << DAISY_REG_VALUE_SHIFT) | (0x32C << DAISY_REG_SHIFT) | 1)},
    {GPIO_AD_B1_09, PWM_5,  ((0U << DAISY_REG_VALUE_SHIFT) | (0x33C << DAISY_REG_SHIFT) | 1)},
    {GPIO_AD_B1_10, PWM_7,  ((0U << DAISY_REG_VALUE_SHIFT) | (0x330 << DAISY_REG_SHIFT) | 1)},
    {GPIO_AD_B1_11, PWM_8,  ((0U << DAISY_REG_VALUE_SHIFT) | (0x340 << DAISY_REG_SHIFT) | 1)},
    {GPIO_AD_B1_12, PWM_10, ((0U << DAISY_REG_VALUE_SHIFT) | (0x334 << DAISY_REG_SHIFT) | 6)},
    {GPIO_AD_B1_13, PWM_11, ((0U << DAISY_REG_VALUE_SHIFT) | (0x344 << DAISY_REG_SHIFT) | 6)},
    {NC   , NC    , 0}
};
