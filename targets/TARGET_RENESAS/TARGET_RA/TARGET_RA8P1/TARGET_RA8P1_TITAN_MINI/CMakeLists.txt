# Copyright (c) 2020-2021 ARM Limited. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(mbed-ra8p1-titan-mini INTERFACE)
target_include_directories(mbed-ra8p1-titan-mini
    INTERFACE
        .
)
target_sources(mbed-ra8p1-titan-mini
    INTERFACE
        board_init.c
        common_data.c
        hal_data.c
        pin_data.c
        hal_warmstart.c
        vector_data.c
        PeripheralPins.c
)
target_link_libraries(mbed-ra8p1-titan-mini INTERFACE mbed-ra8p1)
