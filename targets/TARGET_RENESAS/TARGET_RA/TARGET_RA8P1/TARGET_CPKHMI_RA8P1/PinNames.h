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

#ifndef MBED_PINNAMES_H
#define MBED_PINNAMES_H

#include "cmsis.h"
#include "PinNamesTypes.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PIN_INPUT,
    PIN_OUTPUT
} PinDirection;

/* R7KA8P1KFLCAC, 289-pin BGA package. */
typedef enum {
    P0_0 = 0x0000, P0_1, P0_2, P0_3, P0_4, P0_5, P0_6, P0_7, P0_8, P0_9, P0_10, P0_11, P0_12, P0_13, P0_14, P0_15,
    P1_0 = 0x0100, P1_1, P1_2, P1_3, P1_4, P1_5, P1_6, P1_7, P1_8, P1_9, P1_10, P1_11, P1_12, P1_13, P1_14, P1_15,
    P2_0 = 0x0200, P2_1, P2_2, P2_3, P2_4, P2_5, P2_6, P2_7, P2_8, P2_9, P2_10, P2_11, P2_12, P2_13, P2_14, P2_15,
    P3_0 = 0x0300, P3_1, P3_2, P3_3, P3_4, P3_5, P3_6, P3_7, P3_8, P3_9, P3_10, P3_11, P3_12, P3_13, P3_14, P3_15,
    P4_0 = 0x0400, P4_1, P4_2, P4_3, P4_4, P4_5, P4_6, P4_7, P4_8, P4_9, P4_10, P4_11, P4_12, P4_13, P4_14, P4_15,
    P5_0 = 0x0500, P5_1, P5_2, P5_3, P5_4, P5_5, P5_6, P5_7, P5_8, P5_9, P5_10, P5_11, P5_12, P5_13, P5_14, P5_15,
    P6_0 = 0x0600, P6_1, P6_2, P6_3, P6_4, P6_5, P6_6, P6_7, P6_8, P6_9, P6_10, P6_11, P6_12, P6_13, P6_14, P6_15,
    P7_0 = 0x0700, P7_1, P7_2, P7_3, P7_4, P7_5, P7_6, P7_7, P7_8, P7_9, P7_10, P7_11, P7_12, P7_13, P7_14, P7_15,
    P8_0 = 0x0800, P8_1, P8_2, P8_3, P8_4, P8_5, P8_6, P8_7, P8_8, P8_9, P8_10, P8_11, P8_12, P8_13, P8_14, P8_15,
    P9_0 = 0x0900, P9_1, P9_2, P9_3, P9_4, P9_5, P9_6, P9_7, P9_8, P9_9, P9_10, P9_11, P9_12, P9_13, P9_14, P9_15,
    P10_0 = 0x0A00, P10_1, P10_2, P10_3, P10_4, P10_5, P10_6, P10_7, P10_8, P10_9, P10_10, P10_11, P10_12, P10_13, P10_14, P10_15,
    P11_0 = 0x0B00, P11_1, P11_2, P11_3, P11_4, P11_5, P11_6, P11_7, P11_8, P11_9, P11_10, P11_11, P11_12, P11_13, P11_14, P11_15,
    P12_0 = 0x0C00, P12_1, P12_2, P12_3, P12_4, P12_5, P12_6, P12_7, P12_8, P12_9, P12_10, P12_11, P12_12, P12_13, P12_14, P12_15,
    P13_0 = 0x0D00, P13_1, P13_2, P13_3, P13_4, P13_5, P13_6, P13_7, P13_8, P13_9, P13_10, P13_11, P13_12, P13_13, P13_14, P13_15,

    // mbed Pin Names
#define LED1 P1_10
#define BUTTON0 P3_3
#define BUTTON1 P2_1

    /* UART on JLINK OB */
    CONSOLE_TX = P2_9,
    CONSOLE_RX = P2_8,

    // Not connected
    NC = (int)0xFFFFFFFF
} PinName;

typedef enum {
    PullUp = 0,
    PullDown = 3,
    PullNone = 2,
    OpenDrain = 4,
    PullDefault = PullNone
} PinMode;

#ifdef __cplusplus
}
#endif

#endif
