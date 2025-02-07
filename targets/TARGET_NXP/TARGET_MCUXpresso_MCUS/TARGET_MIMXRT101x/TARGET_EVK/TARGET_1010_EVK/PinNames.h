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

/* MBED TARGET LIST: MIMXRT1020_EVK */

#ifndef MBED_PINNAMES_H
#define MBED_PINNAMES_H

#include "cmsis.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PIN_INPUT,
    PIN_OUTPUT
} PinDirection;

#define GPIO_PORT_SHIFT      12
#define GPIO_MUX_PORT        5

typedef enum {
    PMIC_ON_REQ   = ((5 << GPIO_PORT_SHIFT) | 0),

    GPIO_00 = ((1 << GPIO_PORT_SHIFT) | 0),
    GPIO_01 = ((1 << GPIO_PORT_SHIFT) | 1),
    GPIO_02 = ((1 << GPIO_PORT_SHIFT) | 2),
    GPIO_03 = ((1 << GPIO_PORT_SHIFT) | 3),
    GPIO_04 = ((1 << GPIO_PORT_SHIFT) | 4),
    GPIO_05 = ((1 << GPIO_PORT_SHIFT) | 5),
    GPIO_06 = ((1 << GPIO_PORT_SHIFT) | 6),
    GPIO_07 = ((1 << GPIO_PORT_SHIFT) | 7),
    GPIO_08 = ((1 << GPIO_PORT_SHIFT) | 8),
    GPIO_09 = ((1 << GPIO_PORT_SHIFT) | 9),
    GPIO_10 = ((1 << GPIO_PORT_SHIFT) | 10),
    GPIO_11 = ((1 << GPIO_PORT_SHIFT) | 11),
    GPIO_12 = ((1 << GPIO_PORT_SHIFT) | 12),
    GPIO_13 = ((1 << GPIO_PORT_SHIFT) | 13),

    GPIO_AD_00 = ((1 << GPIO_PORT_SHIFT) | 14),
    GPIO_AD_01 = ((1 << GPIO_PORT_SHIFT) | 15),
    GPIO_AD_02 = ((1 << GPIO_PORT_SHIFT) | 16),
    GPIO_AD_03 = ((1 << GPIO_PORT_SHIFT) | 17),
    GPIO_AD_04 = ((1 << GPIO_PORT_SHIFT) | 18),
    GPIO_AD_05 = ((1 << GPIO_PORT_SHIFT) | 19),
    GPIO_AD_06 = ((1 << GPIO_PORT_SHIFT) | 20),
    GPIO_AD_07 = ((1 << GPIO_PORT_SHIFT) | 21),
    GPIO_AD_08 = ((1 << GPIO_PORT_SHIFT) | 22),
    GPIO_AD_09 = ((1 << GPIO_PORT_SHIFT) | 23),
    GPIO_AD_10 = ((1 << GPIO_PORT_SHIFT) | 24),
    GPIO_AD_11 = ((1 << GPIO_PORT_SHIFT) | 25),
    GPIO_AD_12 = ((1 << GPIO_PORT_SHIFT) | 26),
    GPIO_AD_13 = ((1 << GPIO_PORT_SHIFT) | 27),
    GPIO_AD_14 = ((1 << GPIO_PORT_SHIFT) | 28),

    GPIO_SD_00 = ((2 << GPIO_PORT_SHIFT) | 0),
    GPIO_SD_01 = ((2 << GPIO_PORT_SHIFT) | 1),
    GPIO_SD_02 = ((2 << GPIO_PORT_SHIFT) | 2),
    GPIO_SD_03 = ((2 << GPIO_PORT_SHIFT) | 3),
    GPIO_SD_04 = ((2 << GPIO_PORT_SHIFT) | 4),
    GPIO_SD_05 = ((2 << GPIO_PORT_SHIFT) | 5),
    GPIO_SD_06 = ((2 << GPIO_PORT_SHIFT) | 6),
    GPIO_SD_07 = ((2 << GPIO_PORT_SHIFT) | 7),
    GPIO_SD_08 = ((2 << GPIO_PORT_SHIFT) | 8),
    GPIO_SD_09 = ((2 << GPIO_PORT_SHIFT) | 9),
    GPIO_SD_10 = ((2 << GPIO_PORT_SHIFT) | 10),
    GPIO_SD_11 = ((2 << GPIO_PORT_SHIFT) | 11),
    GPIO_SD_12 = ((2 << GPIO_PORT_SHIFT) | 12),
    GPIO_SD_13 = ((2 << GPIO_PORT_SHIFT) | 13),

    // USB Pins
    CONSOLE_TX = GPIO_10,
    CONSOLE_RX = GPIO_09,

    // Arduino Uno Headers
    ARDUINO_UNO_D0 = GPIO_09,
    ARDUINO_UNO_D1 = GPIO_10,
    ARDUINO_UNO_D2 = GPIO_AD_05,
    ARDUINO_UNO_D3 = GPIO_AD_06,
    ARDUINO_UNO_D4 = GPIO_08,
    ARDUINO_UNO_D5 = GPIO_01,
    ARDUINO_UNO_D6 = GPIO_AD_01,
    ARDUINO_UNO_D7 = GPIO_AD_02,
    ARDUINO_UNO_D8 = GPIO_SD_02,
    ARDUINO_UNO_D9 = GPIO_03,
    ARDUINO_UNO_D10 = GPIO_AD_05,
    ARDUINO_UNO_D11 = GPIO_AD_04,
    ARDUINO_UNO_D12 = GPIO_AD_03,
    ARDUINO_UNO_D13 = GPIO_AD_06,

    ARDUINO_UNO_D14 = GPIO_01,
    ARDUINO_UNO_D15 = GPIO_02,

    ARDUINO_UNO_A0 = GPIO_AD_07,
    ARDUINO_UNO_A1 = GPIO_AD_09,
    ARDUINO_UNO_A2 = GPIO_AD_10,
    ARDUINO_UNO_A3 = GPIO_AD_14,
    ARDUINO_UNO_A4 = GPIO_AD_01,
    ARDUINO_UNO_A5 = GPIO_AD_02,

    // Other Pins
    CUR_A = GPIO_AD_01,
    CUR_B = GPIO_AD_02,
    CUR_C = GPIO_AD_07,
    VOLT_DCB = GPIO_AD_09,
    CUR_DCB = GPIO_AD_10,
    PWM_AT = GPIO_02,
    PWM_AB = GPIO_01,
    PWM_BT = GPIO_04,
    PWM_BB = GPIO_03,
    PWM_CT = GPIO_06,
    PWM_CB = GPIO_05,
    ENC_A = GPIO_AD_05,
    ENC_B = GPIO_AD_06,
    LED_GREEN = GPIO_11,
    MCK = GPIO_08,
    SCK_RX = GPIO_01,
    WS_RX = GPIO_02,
    SD_RX = GPIO_03,
    SCK_TX = GPIO_06,
    WS_TX = GPIO_07,
    SD_TX = GPIO_04,

    // Not connected
    NC = (int)0xFFFFFFFF
} PinName;

// Standardized LED and button names
#define LED1 		LED_GREEN		// USER LED (green)
#define USER_LED	LED1
#define BUTTON1     GPIO_SD_05
#define USER_BUTTON BUTTON1

typedef enum {
    PullNone = 0,
    PullDown = 1,
    PullUp_47K  = 2,
    PullUp_100K = 3,
    PullUp_22K  = 4,
    PullDefault = PullUp_47K,
    PullUp = PullUp_47K
} PinMode;

#ifdef __cplusplus
}
#endif

#endif
