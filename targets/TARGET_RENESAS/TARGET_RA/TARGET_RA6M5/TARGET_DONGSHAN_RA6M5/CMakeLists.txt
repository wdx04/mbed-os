# Copyright (c) 2020-2021 ARM Limited. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(mbed-dongshan-ra6m5 INTERFACE)
target_include_directories(mbed-dongshan-ra6m5
    INTERFACE
        .
)
target_sources(mbed-dongshan-ra6m5
    INTERFACE
        board_init.c
        common_data.c
        hal_data.c
        pin_data.c
        vector_data.c
        PeripheralPins.c
)
target_link_libraries(mbed-dongshan-ra6m5 INTERFACE mbed-ra6m5)
