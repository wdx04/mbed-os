# Copyright (c) 2021 ARM Limited. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

# MIMXRT1010_EVK
add_library(mbed-mimxrt1010-evk INTERFACE)

target_include_directories(mbed-mimxrt1010-evk
    INTERFACE
        .
        TARGET_1010_EVK
)

target_sources(mbed-mimxrt1010-evk
    INTERFACE
        fsl_flexspi_nor_boot.c
		flash_api.c

        TARGET_1010_EVK/PeripheralPins.c
        TARGET_1010_EVK/xip/evkmimxrt1010_flexspi_nor_config.c
)

target_link_libraries(mbed-mimxrt1010-evk
	INTERFACE
		mbed-mimxrt101x
)
