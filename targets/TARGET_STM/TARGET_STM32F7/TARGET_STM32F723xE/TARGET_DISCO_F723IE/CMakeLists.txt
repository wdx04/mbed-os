# Copyright (c) 2020 ARM Limited. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(mbed-disco-f723ie INTERFACE)

target_sources(mbed-disco-f723ie
    INTERFACE
        PeripheralPins.c
        system_clock.c
        TargetInit.c
)

target_include_directories(mbed-disco-f723ie
    INTERFACE
        .
)

target_link_libraries(mbed-disco-f723ie INTERFACE mbed-stm32f723xe)
